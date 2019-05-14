#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "socket_client.h"

#define SRV_PORT	"4715"
#define MAXDATASIZE 1000

struct transfer {
	bool putget;
	char name[20];
	FILE* payload;
}

bool connected = false;
int sockfd;

int main()
{
	while(1)
	{
		char* linebuf = NULL;
		size_t linesize = 0;

		if(getline(&linebuf, &linesize, stdin) == -1)
		{
		    perror("Failed to get input");
		    fflush(stderr);
		    free(linebuf);
		    linebuf = NULL;
		    return -1;
		}

		size_t linebuflen = strlen(linebuf);
		if(linebuf[linebuflen - 1] == '\n')
		{
		    linebuf[linebuflen - 1] = '\0';
		}
		fflush(stdout);

		char* nextToken = NULL;
		char* originalmsg = malloc(strlen(linebuf));
		strcpy(originalmsg, linebuf);
		nextToken = strtok(linebuf, " ");

		// Check what command was entered
		if(strcmp(linebuf, "connect") == 0) {
			// Connect if we aren't, do nothing otherwise
			if (connected) {
				fprintf(stderr, "We are already connected\n");
				fflush(stderr);
			} else {
				// Get second argument (address)
				nextToken = strtok(NULL, " ");
				if (isValidArgument(nextToken)) {
					sockfd = connectToServer(nextToken);

					if (sockfd == -1) {
						fprintf(stderr, "Connection was refused.\n");
						fflush(stderr);
					} else {
						connected = true;
						puts("Connection was established!");
						fflush(stdout);
					}
				} else {
				    fprintf(stderr, "Argument missing\n");
				    fflush(stderr);
				}
			}
		} else if(strcmp(linebuf, "put") == 0) {
			// Get second argument (file)
			nextToken = strtok(NULL, " ");
			if(isValidArgument(nextToken)) {
				// Try sending file to server
				sendFile(nextToken);
			} else {
				fprintf(stderr, "Argument missing\n");
				fflush(stderr);
			}
		}
		else if (strcmp(linebuf, "get") == 0) {
			// Get second argument (file)
			nextToken = strtok(NULL, " ");

			if(isValidArgument(nextToken)) {
				getFile(nextToken);
			} else {
				fprintf(stderr, "Argument missing\n");
				fflush(stderr);
			}
		} else if (strcmp(linebuf, "quit") == 0) {
			// Disconnect and quit program
			disconnect();
			exit(EXIT_SUCCESS);
		} else {
		    puts("Unknown command");
		    fflush(stdout);
		}
		
		free(linebuf);
		linebuf = NULL;
		free(originalmsg);
		originalmsg = NULL;
	}
}


/*
 * Check if argument is not null or length 0
 */
bool isValidArgument(char* str)
{
	bool valid = (str != NULL) && (strlen(str) > 0);
	return valid;
}


/*
 * Connect to a server with the given address
 */
int connectToServer(char* address) {
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
    int status = getaddrinfo(address, SRV_PORT, &hints, &res);
    if(0 != status)
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

/*
 * Disconnect from server
 */
void disconnect() {
	if (connected) {
		close(sockfd);
		puts("disconnected");
		fflush(stdout);
	} else {
		puts("We aren't connected");
		fflush(stdout);
	}
}


/*
 * Send a file to server
 */
void sendFile(char* filename) {
	struct transfer t;
	FILE* f;
	t.putget = true;
	t.payload = f;
	
	printf("sent %s to server\n", filename);
	fflush(stdout);
}


/*
 * Get a file from server
 */
void getFile(char* filename) {
	struct transfer t;
	t.putget = true;
	strcpy(&t.name, filename);
	
	printf("got %s from server\n", filename);
	fflush(stdout);
}