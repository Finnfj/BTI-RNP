#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "chatConnection.h"
#include "client.h"
#include "server.h"
#include "peerRole.h"

void* initAndListen(void*);

void* sharedsockfd = NULL;                 //This pointer is used to pass the socket descriptor to the listening thread. Global variable because the function passing it to the thread as a local variable isn't feasible, since scope is exited after pthread_create
pthread_t listenerThread = NULL;           //This thread listens for messages while connected to a peer
pthread_t connectionListenerThread = NULL; //This thread listens for incoming connections when we are hosting
bool isListening = false;                  //This is a custom flag supposed to hold record, whether the thread listening on incoming conenctions is still running.
bool isConnected = false;                  //This is a custom flag supposed to hold record, whether the thread listening on incoming messages is still running.
int currentsockfd = 0;                     //This variable is supposed to hold the current socket descriptor of the connected peer.
int listeningsockfd = 0;                   //This thread is supposed to hold the internal socket, on which this machine is listening to accept incoming connections. If user decides to terminate waiting on incoming connections, this descriptor needs to be closed in order to be able to reuse the port.
enum PEERROLE role = NONE;                 //This enum holds the role this machine is currently participating at.

void connectToPeer(char* address)
{
    if(!isConnected)
    {
        int initfd = init_client(address);
        if(initfd == -1)
        {
            fprintf(stderr, "Connection refused. No connection was established.\n");
            fflush(stderr);
        }
        else
        {
            isConnected = 1;
            currentsockfd = initfd;
            role = CLIENT;
            puts("Connection established!");
            fflush(stdout);
        }
    }
    else
    {
        fprintf(stderr, "A connection is already present. Disconnect to establish a new connection.\n");
        fflush(stderr);
    }
}

void disconnectFromPeer()
{
    if(isConnected)
    {
        //Attempt to stop listener thread
        if(role == SERVER)
        {
            if(pthread_cancel(listenerThread) != 0)
            {
                perror("Error calling cancel on listener thread");
                fflush(stderr);
            }
            else
            {
                void* res = NULL;
                if(pthread_join(listenerThread, &res) != 0)
                {
                    perror("pthread_join(listenerthread) returned error");
                    fflush(stderr);
                }
                else
                {
                    if(res == PTHREAD_CANCELED)
                    {
                        puts("Listener thread stopped.");
                        fflush(stdout);
                        listenerThread = NULL;

//                        free(sharedsockfd); //TODO why does this make the program crash?
                        sharedsockfd = NULL;
                    }
                    else
                    {
                        perror("Listener Thread could not be stopped!!");
                        fflush(stderr);
                    }
                }
            }
        }
        close(currentsockfd);
        currentsockfd = 0;
        isConnected = 0;
        role = NONE;
        puts("Disconnected.\n");
        fflush(stdout);
    }
    else
    {
        if(isListening)
        {
            int cancelstatus = 0;
            if((cancelstatus = pthread_cancel(connectionListenerThread)) != 0)
            {
                if(cancelstatus == ESRCH)
                {
                    puts("Not listening at all.");
                    fflush(stdout);
                }
                else
                {
                    perror("Error calling cancel on connectionlistener thread");
                    fflush(stderr);
                }
            }
            else
            {
                void* res = NULL;
                if(pthread_join(connectionListenerThread, &res) != 0)
                {

                    perror("pthread_join(connectionlistenerthread) returned error");
                    fflush(stderr);

                }
                else
                {
                    if(res == PTHREAD_CANCELED)
                    {
                        isListening = false;
                        connectionListenerThread = NULL;
                        printf("Server was listening on socket %i. Closing...\n",listeningsockfd);
                        close(listeningsockfd);
                        listeningsockfd = 0;
                        puts("Connection-Listener thread stopped.");
                        fflush(stdout);
                    }
                    else
                    {
                        perror("Connection-Listener Thread could not be stopped!!");
                        fflush(stderr);
                    }
                }
            }
        }
        else
        {
            fprintf(stderr, "No connection present to disconnect.\n");
            fflush(stderr);
        }
    }
}
void sayToPeer(char* message)
{
    if(isConnected)
    {
        if(role == SERVER)
        {
            fprintf(stderr, "This function is not supported as a server role yet\n");
            fflush(stderr);
            return;
        }
        if(sendTextMessage(message, currentsockfd) != 0)
        {
            disconnectFromPeer();
        }
    }
    else
    {
        fprintf(stderr, "No connection present to send to.\n");
        fflush(stderr);
    }
}
void sendToPeer(char* fileName)
{
//    if(isConnected)
//    {
//        printf("Sending file %s.\n", fileName);
//        fflush(stdout);
//    }
//    else
//    {
//        fprintf(stderr, "No connection present to send to.\n");
//        fflush(stderr);
//    }
}

void hostConnection()
{
    if(!isConnected)
    {
        if(pthread_create(&connectionListenerThread, NULL, initAndListen, &isListening) != 0)
        {
            perror("Server Init call failed");
            fflush(stderr);
        }
        else
        {
            isListening = true;
        }
    }
    else
    {
        fprintf(stderr, "A connection is already present. Disconnect to establish a new connection.\n");
        fflush(stderr);
    }
}

void quit()
{
    if(isConnected)
    {
        disconnectFromPeer();
    }
    puts("Shut down.\n");
    fflush(stdout);
    exit(EXIT_SUCCESS);
}

//This is a thread
void* initAndListen(void* isRunning)
{
    bool* isthisrunning = (bool*)(isRunning);

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL); //No worries; accept is a cancellation point (see manpages on pthreads(7))

    int initfd = init_server(&listeningsockfd); //This may block indefinetely
    if(initfd == -1)
    {
        fprintf(stderr, "Connection refused as server. No connection was established.\n");
        fflush(stderr);
    }
    else
    {
        isConnected = 1;
        currentsockfd = initfd;
        role = SERVER;
        sharedsockfd = malloc(sizeof(int));
        memcpy(sharedsockfd, &currentsockfd, sizeof(int));
        if(pthread_create(&listenerThread, NULL, listenToMessages, sharedsockfd) != 0)
        {
            perror(
                    "Message Listener thread could not be started.\nConnection established, but currently not able to receive messages\nUser should disconnect.");
            fflush(stderr);
        }
        else
        {
            puts("Connection established!");
            fflush(stdout);
        }
    }
    *isthisrunning = false;
    return NULL;
}
