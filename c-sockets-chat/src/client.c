#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "client.h"

#define PORT "4713"

/**
 * This function initializes a connection and returns the socket descriptor over which to send and receive data.
 */
int init_client(char* node)
{
    //Crietria for addresses we want to get from getaddrinfo
    struct addrinfo hints;
    //Put the result from getaddrinfo here
    struct addrinfo* res;
    //A pointer to iterate through the addresses returned by getaddrinfo
    struct addrinfo* p;

    memset(&hints, 0, sizeof(hints));
    //ipv4 or ipv6
    hints.ai_family = AF_UNSPEC;
    //We want TCP!
    hints.ai_socktype = SOCK_STREAM;

    int status = 0;
    status = getaddrinfo(node, PORT, &hints, &res);

    if(status != 0)
    {
        fprintf(stderr, "Failed to get address info: %s\n", gai_strerror(status)); //gai_strerror is a special form of streror, which interpretes the speial error codes returned by getaddrinfo.
        fflush(stderr);
        return -1;
    }

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

        if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("Couldn't connect");
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

    //Connection succeeded!

    //Let's look at the address of our peer.

    if(p->ai_family == AF_INET)
    {
        struct in_addr* peerAddress = &(((struct sockaddr_in*) (p->ai_addr))->sin_addr);
        char addrstrbuf[INET_ADDRSTRLEN] = "";
        inet_ntop(AF_INET, peerAddress, addrstrbuf, sizeof(addrstrbuf));
        printf("Connected to %s!\n", addrstrbuf);
        fflush(stdout);
    }
    else //if( p->ai_family == AF_INET6 )
    {
        struct in6_addr* peerAddress = &(((struct sockaddr_in6*) (p->ai_addr))->sin6_addr);
        char addrstrbuf[INET6_ADDRSTRLEN] = "";
        inet_ntop(AF_INET, peerAddress, addrstrbuf, sizeof(addrstrbuf));
        printf("Connected to %s!\n", addrstrbuf);
        fflush(stdout);
    }

    freeaddrinfo(res);

    return sockfd;
}

int sendTextMessage(char* message, int sendsockfd)
{
    int transmittedbytes = send(sendsockfd, message, strlen(message), 0);
    if(transmittedbytes <= 0)
    {
        perror("Failed to send data. Disconnecting...");
        fflush(stderr);
        return -1;
    }
    else
    {
//        printf("%i bytes of data were sent\n", transmittedbytes);
//        fflush(stdout);
        //We have sent our message.
        //We expect our peer to send a "1" to confirm they have received our message.
        int ack = 0;
        int receivedbytes = recv(sendsockfd, &ack, sizeof(ack), 0);
        if(receivedbytes == 0)
        {
            perror("Peer has already disconnected. Disconnecting...");
            fflush(stderr);
            return -1;
        }
        else if(receivedbytes < 0)
        {
            perror("Failed to receive acknowledgement from peer. Disconnecting...");
            fflush(stderr);
            return -1;
        }
        else
        {
            if(ack == 1)
            {
                printf("[ME] %s\n", message);
                fflush(stdout);
                return 0;
            }
            else
            {
                perror("Received ill-formed acknowledgment. Disconnecting...");
                fflush(stderr);
                return -1;
            }
        }
    }
}
