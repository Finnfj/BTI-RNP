#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netdb.h>
#include <errno.h>
#include "socket_srv.h"

#include <sys/select.h>
#include <fcntl.h>

#define SRV_PORT	"4715"
#define MAXDATASIZE 1000
#define MAXCLIENTS  5


struct transfer {
	bool putget;
	char name[20];
	FILE* payload;
}

// manage client addresses
struct sockaddr_ref {
	bool occupied;
	int socknum;
	struct sockaddr_storage cliaddr;
};

int s_tcp;	// Main socket descriptor
struct sockaddr_ref cliaddresses[MAXCLIENTS];   // Store client informations


// cleanup
void free_sock(void) {
    close(s_tcp);
    for (int j=0; j < MAXCLIENTS; j++) {
	if (cliaddresses[j].occupied) {
        	close(cliaddresses[j].socknum);
	}
    }
}

/**
 * This function initializes a socket, which listens on incoming connections on SRV_PORT.
 */
int main ( )
{
    // Clean up on exit if possible
    atexit(free_sock);
    
    int newss;			// socket descriptor
    int maxfd, n;               // Main loop values
    char buffer[MAXDATASIZE];   // buffer
    struct addrinfo hints;      // getaddrinfo parameters
    struct addrinfo* res;       // getaddrinfo result
    struct addrinfo* p;         // getaddrinfo iteration pointer
    
    fd_set master, readfds;    // Filedescriptor sets for non-blocking multiplexing
    
    struct sockaddr_storage *cliaddrp;              // Pointer to a cliaddr from cliaddresses
    socklen_t cliaddrlen;                           // Set later due to possible padding causing different lengths, means sizeof(cliaddr)
    
	
    memset(&cliaddresses, 0, sizeof(cliaddresses));
    memset(&hints, 0, sizeof(hints));
	
    // either IPv4/IPv6
    hints.ai_family = AF_UNSPEC;
    // TCP only
    hints.ai_socktype = SOCK_STREAM;
    // Host
    hints.ai_flags = AI_PASSIVE;
    
    
    // get addresses    
    int status = 0;
    status = getaddrinfo(NULL, SRV_PORT, &hints, &res);
    if (0 != status)
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

        // Set socket option to reuse local addresses
        int par = 1;
        if(-1 == setsockopt(s_tcp, SOL_SOCKET, SO_REUSEADDR, &par, sizeof(int)))
        {
            close(s_tcp);
            perror("Error setting socket option");
            fflush(stderr);
            return -1;
        }

        if(-1 == bind(s_tcp, p->ai_addr, p->ai_addrlen))
        {
            close(s_tcp);
            perror("Error binding socket");
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
    
    // Release memory used for results
    freeaddrinfo(res);
    
    // Wait for incoming connections
    puts("Start listening for connections...");
    printf("Listening on socket %i\n", s_tcp);
    fflush(stdout);
    
    // Listen on socket
    if (-1 == listen(s_tcp, MAXCLIENTS))
    {
        perror("Error listening");
        fflush(stderr);
        return -1;
    }
    
    // Configure socket file descriptor to non-blocking
    if (-1 == (fcntl(s_tcp, F_SETFD, O_NONBLOCK)))
    {
        perror("Error changing socketfd to non-blocking");
        fflush(stderr);
        return -1;
    }
    
    // Empty fd sets
    FD_ZERO(&master);
    FD_ZERO(&readfds);
    
    // Add hosting socket to main file descriptor
    FD_SET(s_tcp, &master);
    
    // Need to always know the maximum integer value of all file descriptors, starts with hosting socket fd
    maxfd = s_tcp;
    
    
    // Main iteration loop
    while (1)
    {
        // Copy main file descriptor to read-fd
        memcpy(&readfds, &master, sizeof(master));

        // Enable read fds and get their number
        if (-1 == (n = select(maxfd+1, &readfds, NULL, NULL, NULL)))
        {
            perror("Error selecting");
            fflush(stderr);
            return -1;
        }

        printf("No of ready descriptor: %d\n", n);
        fflush(stdout);

        // iterate socket fds and work with them
        for (int i=0; i<=maxfd && n>0; i++)
        {
            // only do something with socket fds we own
            if (FD_ISSET(i, &readfds))
            {
                // process one fd, decrease number of ready fds we need to process
                n--;
                
                // is this our main socket? If so try to accept news connections
                if (i == s_tcp)
                {
                    // find a free slot for our cliaddr
                    int freeslot = 0;
                    int j;
                    
                    for (j=0; j < MAXCLIENTS; j++) {
                        if (false == cliaddresses[j].occupied)
                        {
                            // Set pointer to free slots cliaddr
                            cliaddrp = &cliaddresses[j].cliaddr;
                            freeslot = 1;
                            break;
                        }
                    }
                    
                    // accept if there is a free client slot
                    if (freeslot == 1)
                    {
                        socklen_t cliaddrlen = sizeof(*cliaddrp);
                        if (-1 == (newss = accept(s_tcp, cliaddrp, &cliaddrlen)))
                        {
                            if (EWOULDBLOCK != errno)
                            {
                                perror("Error different than expected EWOULDBLOCK errno occured");
                            }
                            break;
                        }
                        else    // successfully accepted new socket
                        {
                            // set socket reference in our cliaddresses
                            cliaddresses[j].socknum = newss;
			    cliaddresses[j].occupied = true;
                            
				
			    struct sockaddr* q = (struct sockaddr*)  cliaddrp;

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
			    
			    
                            // Configure socket file descriptor to non-blocking
                            if (-1 == (fcntl(newss, F_SETFD, O_NONBLOCK)))
                            {
                                perror("Error changing comm fd to non-blocking");
                                fflush(stderr);
                            }

                            // Add new socket to our main file descriptor
                            FD_SET(newss, &master);
    
                            // update max fd number if higher
                            if (maxfd < newss)
                            {
                                maxfd = newss;
                            }
                        }
                    }
                }
                else    // Communication socket, receive data
                {
		    int nbytes;
                    nbytes = recv(i, buffer, sizeof(buffer), 0);
                    if (nbytes <= 0)
                    {
                        if (EWOULDBLOCK != errno)
                        {
                            perror("Error different than expected EWOULDBLOCK errno occured");
                            fflush(stderr);
                        }
                        break;
                    }
		    
		    
                    struct transfer t;
		    t = (struct transfer) buffer;
		    
		    if (t.putget) {
			// Client sending file, receive and save
			FILE* f = t.payload;
		    } else {
			// Client requests file, open and send
			FILE* f;
		    }
		    
                    // Append EOS
                    // buffer[nbytes] = '\0';
                    
                    // Print data
                    //printf("%s\n", buffer);
                    printf("%i bytes received.\n", nbytes);
                    fflush(stdout);
                }
            }
        }
    }
}
