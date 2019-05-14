#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#include "server.h"

#define PORT "4713"
#define MAXDATASIZE 1000

/**
 * This function initializes a socket, which listens on incoming connections on PORT.
 */
int init_server(int* sockfdp)
{
    //Crietria for addresses we want to get from getaddrinfo
    struct addrinfo hints;
    //Put the result from getaddrinfo here
    struct addrinfo* res;
    //A pointer to iterate through the addresses returned by getaddrinfo
    struct addrinfo* p;
    //We will store the address info of a client, that wants to connect with us, here, later
    struct sockaddr_storage cliaddr;

    memset(&hints, 0, sizeof(hints));
    //ipv4 or ipv6
    hints.ai_family = AF_UNSPEC;
    //We want TCP!
    hints.ai_socktype = SOCK_STREAM;
    //We are hosting
    hints.ai_flags = AI_PASSIVE;

    int status = 0;
    status = getaddrinfo(NULL, PORT, &hints, &res);

    if(status != 0)
    {
        fprintf(stderr, "Failed to get address info FOR MYSELF: %s\n", gai_strerror(status)); //gai_strerror is a special form of streror, which interpretes the speial error codes returned by getaddrinfo.
        fflush(stderr);
        return -1;
    }

    //This socket is only used to look for incoming connections.
    int sockfd = 0;

    for(p = res; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(sockfd == -1)
        {
            perror("Couldn't create socket with address");
            fflush(stderr);
            continue;
        }

        //Not have to wait for a minute EVERYTIME our program crashes
        int yes = 1;
        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            close(sockfd);
            perror("Could not set socket options for reuse");
            fflush(stderr);
            return -1;
        }

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("Couldn't bind");
            fflush(stderr);
            continue;
        }
        break;
    }

    if(p == NULL)
    {
        fprintf(stderr, "Connection attempt failed\n");
        fflush(stderr);
        return -1;
    }

    freeaddrinfo(res);
    *sockfdp = sockfd;
    //Everything is set up! Let's wait for incoming connections.
    puts("Now waiting for incoming connections...");
    fflush(stdout);
    printf("Listening on socket %i\n", sockfd);
    fflush(stdout);

    //Marks sockfd for listen
    if(listen(sockfd, 5) == -1)
    {
        perror("Couldn't listen");
        fflush(stderr);
        return -1;
    }
    socklen_t cliaddrlen = sizeof(cliaddr);
    //This socket is used for actual communication
    int commsockfd = 0;

    //The accept call blocks until a client wants to connect with us.
    commsockfd = accept(sockfd, (struct sockaddr*) &cliaddr, &cliaddrlen);
    if(commsockfd == -1)
    {
        perror("Incoming connection could not be accepted");
        fflush(stderr);
        return -1;
    }

    //We are now connected with a client!
    //No longer need to listen on incoming connections.
    close(sockfd);
    //Let's look at the address of our peer.

    struct sockaddr* q = (struct sockaddr*) &cliaddr;

    if(q->sa_family == AF_INET)
    {
        struct in_addr* peerAddress = &(((struct sockaddr_in*)(q))->sin_addr);
        char addrstrbuf[INET_ADDRSTRLEN] = "";
        inet_ntop(AF_INET, peerAddress, addrstrbuf, sizeof(addrstrbuf));
        printf("Connected to %s!\n", addrstrbuf);
        fflush(stdout);
    }
    else //if( q->ai_family == AF_INET6 )
    {
        struct in6_addr* peerAddress = &(((struct sockaddr_in6*)(q))->sin6_addr);
        char addrstrbuf[INET6_ADDRSTRLEN] = "";
        inet_ntop(AF_INET6, peerAddress, addrstrbuf, sizeof(addrstrbuf));
        printf("Connected to %s!\n", addrstrbuf);
        fflush(stdout);
    }

    return commsockfd;
}

//This is a thread
void* listenToMessages(void* listensockfd)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL); //No worries; send and recv are cancellation points (see manpages on pthreads(7))
    puts("Now listening for incoming messages");
    fflush(stdout);

    int* sockfdp = (int*)listensockfd;
    int sockfd = *sockfdp;

    while(1)
    {
        char recvmsg[MAXDATASIZE] = "";
        int receivedbytes = recv(sockfd, recvmsg, sizeof(recvmsg), 0);
        if(receivedbytes == 0)
        {
            perror("Peer has already disconnected. Listenerthread shut down. User should disconnect.");
            fflush(stderr);
            return NULL;
        }
        else if(receivedbytes < 0)
        {
            perror("Failed to receive message from peer. Listenerthread shut down. User should disconnect.");
            fflush(stderr);
            return NULL;
        }
        else
        {
//            printf("%i bytes of data have been received\n", receivedbytes);
            printf("[PEER] %s\n", recvmsg);
            fflush(stdout);
            //We have received a message. Sending acknowledgment to peer.
            int ack = 1;
            int transmittedbytes = send(sockfd, &ack, sizeof(int), 0);
            if(transmittedbytes <= 0)
            {
                perror("Failed to send acknowledgement for received message.\n Listenerthread shutdown. User should disconnect.");
                fflush(stderr);
                return NULL;
            }
            else
            {
                //We acknowledged our reception of the message. Yay!
                continue;
            }
        }
    }
    return NULL;
}
