#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/select.h>
#include <fcntl.h>

#define SRV_PORT	4715
#define MAXDATASIZE 1000
#define MAXCLIENTS  5

// manage client addresses
struct sockaddr_ref {
	int socknum;
	struct sockaddr_storage cliaddr;
};

// cleanup
void free_sock(void) {
    close(s_tcp);
    for (int j=0; j < MAXCLIENTS; j++) {
        if (NULL != cliaddresses[j].cliaddr)
        {
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
    
    int s_tcp, newss;			// socket descriptor
    int maxfd, n;               // Main loop values
    char buffer[MAXDATASIZE];   // buffer
    struct addrinfo hints;      // getaddrinfo parameters
    struct addrinfo* res;       // getaddrinfo result
    struct addrinfo* p;         // getaddrinfo iteration pointer
    
    fd_set master, readfds;    // Filedescriptor sets for non-blocking multiplexing
    
    struct sockaddr_ref cliaddresses[MAXCLIENTS];   // Store client informations
    struct sockaddr_storage *cliaddrp;              // Pointer to a cliaddr from cliaddresses
    socklen_t cliaddrlen;                           // Set later due to possible padding causing different lengths, means sizeof(cliaddr)
    
    memset(&hints, 0, sizeof(hints));
    // either IPv4/IPv6
    hints.ai_family = AF_UNSPEC;
    // TCP only
    hints.ai_socktype = SOCK_STREAM;
    // Host
    hints.ai_flags = AI_PASSIVE;
    
    
    // get addresses    
    if(0 != getaddrinfo(NULL, SRV_PORT, &hints, &res))
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
        int true = 1;
        if(-1 == setsockopt(s_tcp, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)))
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
    if(-1 == listen(s_tcp, MAXCLIENTS))
    {
        perror("Error listening");
        fflush(stderr);
        return -1;
    }
    
    // Configure socket file descriptor to non-blocking
    if (-1 == (fcntl(s_tcp, F_SETFD, O_NONBLOCK)
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
        for (i=0; i<=maxfd && n>0; i++)
        {
            // only do something with socket fds we own
            if (FD_ISSET(i, &readfds))
            {
                // process one fd, decrease number of ready fds we need to process
                n--;
                
                // is this our main socket? If so try to accept news connections
                if (i == sockfd)
                {
                    // find a free slot for our cliaddr
                    int freeslot = 0;
                    int j;
                    
                    for (j=0; j < MAXCLIENTS; j++) {
                        if (NULL == cliaddresses[j].cliaddr)
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
                        if (-1 == (news = accept(sockfd, cliaddrp, &cliaddrlen)))
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
                            cliaddresses[j].socknum = news;
                            
                            // Configure socket file descriptor to non-blocking
                            if (-1 == (fcntl(news, F_SETFD, O_NONBLOCK)))
                            {
                                perror("Error changing comm fd to non-blocking");
                                fflush(stderr);
                            }

                            // Add new socket to our main file descriptor
                            FD_SET(news, &master);
    
                            // update max fd number if higher
                            if (maxfd < news)
                            {
                                maxfd = news;
                            }
                        }
                    }
                }
                else    // Communication socket, receive data
                {
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
                    
                    // Append EOS
                    buffer[nbytes] = '\0';
                    
                    // Print data
                    printf("%s\n", buffer);
                    printf("%zi bytes received.\n", nbytes);
                    fflush(stdout);
                    
                    //uncomment if we want to close connection after file transfer
                    //close(i);
                    //FD_CLR(i, &master);
                }
            }
        }
    }
}
