#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define SRV_PORT	4715
#define MAXDATASIZE 1000

bool connected = false;
int socket;

int main ( )
{
    while(1)
    {
        char* lineBuf = NULL;
        size_t lineSize = 0;

        if(getline(&lineBuf, &lineSize, stdin) == -1)
        {
            perror("Failed to get input");
            fflush(stderr);
            free(lineBuf);
            lineBuf = NULL;
            return -1;
        }

        size_t lineBuflen = strlen(lineBuf);
        if(lineBuf[lineBuflen - 1] == '\n')
        {
            lineBuf[lineBuflen - 1] = '\0';
        }
        fflush(stdout);

        char* nextToken = NULL;
        char* originalmsg = malloc(strlen(lineBuf));
        strcpy(originalmsg, lineBuf);
        nextToken = strtok(lineBuf, " ");

        if(strcmp(lineBuf, "connect") == 0)
        {
            if (connected) {
                fprintf(stderr, "We are already connected\n");
                fflush(stderr);
            } else {
                nextToken = strtok(NULL, " ");
                if(isValidArgument(nextToken))
                {
                    socket = connect(nextToken);
                    
                    if (socket == -1) {
                        fprintf(stderr, "Connection was refused.\n");
                        fflush(stderr);
                    } else {
                        connected = true;
                        puts("Connection was established!");
                        fflush(stdout);
                    }
                }
                else
                {
                    fprintf(stderr, "Argument missing\n");
                    fflush(stderr);
                }
            }
        }
        
        // bis hierhin geupdated
        else if(strcmp(lineBuf, "disconnect") == 0)
        {
            disconnectFromPeer();
        }
        else if(strcmp(lineBuf, "say") == 0)
        {
            nextToken = strtok(NULL, " ");
            if(isValidArgument(nextToken))
            {
                sayToPeer(originalmsg + 4);
            }
            else
            {
                fprintf(stderr, "Argument missing\n");
                fflush(stderr);
            }
        }
        else if(strcmp(lineBuf, "send") == 0)
        {
            nextToken = strtok(NULL, " ");

            if(isValidArgument(nextToken))
            {
                sendToPeer(nextToken);
            }
            else
            {
                fprintf(stderr, "Argument missing\n");
                fflush(stderr);
            }
        }
        else if(strcmp(lineBuf, "host") == 0)
        {
            free(lineBuf);
            lineBuf = NULL;
            hostConnection();
        }
        else if(strcmp(lineBuf, "quit") == 0)
        {
            free(lineBuf);
            lineBuf = NULL;
            quit();
        }
        else
        {
            puts("Unknown command");
            fflush(stdout);
        }
        free(lineBuf);
        lineBuf = NULL;
        free(originalmsg);
        originalmsg = NULL;
    }
}

/*
 * Check if argument is not null or length 0
 */
bool isValidArgument(char* str)
{
    return ((str != NULL) && (strlen(str) > 0));
}

int connect(char* address) {
    int s_tcp;			        // socket descriptor
    struct addrinfo hints;      // getaddrinfo parameters
    struct addrinfo* res;       // getaddrinfo result
    struct addrinfo* p;         // getaddrinfo iteration pointer
    
    memset(&hints, 0, sizeof(hints));
    // either IPv4/IPv6
    hints.ai_family = AF_UNSPEC;
    // TCP only
    hints.ai_socktype = SOCK_STREAM;
    
    // get addresses    
    if(0 != getaddrinfo(address, SRV_PORT, &hints, &res))
    {
        fprintf(stderr, "Error: %s\n", gai_strerror(status)); // gai_strerror is getaddrinfo's Error descriptor
        fflush(stderr);
        return -1;
    }
    
    // Create and bind socket
    for(p = res; p != NULL; p = p->ai_next)
    {
        s_tcp = socket(p->ai_family, p->ai_socktype, IPPROTO_TCP);
        if(-1 == s_tcp)
        {
            perror("Error creating socket");
            fflush(stderr);
            continue;
        }

        if(-1 == connect(s_tcp, p->ai_addr, p->ai_addrlen))
        {
            close(s_tcp);
            perror("Error connecting to socket");
            fflush(stderr);
            continue;
        }
        // One successful socket is enough, go on
        break;
    }
    
    // No network interface was found to run a socket on
    if(NULL == p)
    {
        fprintf(stderr, "No available addresses to host on\n");
        fflush(stderr);
        return -1;
    }
    
    // Check for IPv4/IPv6
    if(p->ai_family == AF_INET)
    {
        struct in_addr* peerAddress = &(((struct sockaddr_in*) (p->ai_addr))->sin_addr);
        char addrstrbuf[INET_ADDRSTRLEN] = "";
        inet_ntop(AF_INET, peerAddress, addrstrbuf, sizeof(addrstrbuf));
        printf("Connected to %s.\n", addrstrbuf);
        fflush(stdout);
    }
    else // AF_INET6
    {
        struct in6_addr* peerAddress = &(((struct sockaddr_in6*) (p->ai_addr))->sin6_addr);
        char addrstrbuf[INET6_ADDRSTRLEN] = "";
        inet_ntop(AF_INET, peerAddress, addrstrbuf, sizeof(addrstrbuf));
        printf("Connected to %s.\n", addrstrbuf);
        fflush(stdout);
    }
    // Connection complete, free addrinfo memory
    freeaddrinfo(res);
    
    return s_tcp;
}
    
/*   int s_tcp;			// socket descriptor 
  struct sockaddr_in sa;	// socket address structure
  int sa_len = sizeof(struct sockaddr_in), n;
  char* msg="Hello World!";


  sa.sin_family = AF_INET;
  sa.sin_port = htons(SRV_PORT);

  if (inet_pton(sa.sin_family,SRV_ADDRESS, &sa.sin_addr.s_addr) <= 0) {
	perror("Address Conversion");
	exit(1);
	}

  if ((s_tcp=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0) {
	perror("TCP Socket");
	exit(1);
	}

  if (connect(s_tcp,(struct sockaddr*) &sa, sa_len) < 0) {
	perror("Connect");
	exit(1);
	}

  if((n=send(s_tcp,msg, strlen(msg),0)) > 0) {
	printf("Message %s sent ( %i Bytes).\n", msg, n); 
	}

  close(s_tcp); */
