/* Server Application
 * Authors Finn Jannsen & Paul Mathia 
 * RNP Aufgabe Prof. Dr. Schmidt 
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdbool.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "socket_srv.h"

#include <sys/select.h>
#include <fcntl.h>

#define SRV_PORT "4715"
#define MAXDATASIZE 1000
#define MAXCLIENTS  5

// manage client addresses
struct sockaddr_ref {
    bool occupied;
    int socknum;
    struct sockaddr_storage cliaddr;
    char* hostname;
};

int s_tcp;					// Main socket descriptor
struct sockaddr_ref cliaddresses[MAXCLIENTS];   // Store client informations
int newss;		    			// socket descriptor
int maxfd, n;               			// Main loop values
char buffer[MAXDATASIZE];   			// buffer
char filenamePut[MAXDATASIZE];		


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
                        if (-1 == (newss = accept(s_tcp,(struct sockaddr*) cliaddrp, &cliaddrlen)))
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
                    memset(buffer, 0, MAXDATASIZE);
                    //read requestflag
                    nbytes = read(i, buffer, MAXDATASIZE);
                    
                    // Is read succesful - If this block is entered it would either block (EWOULDBLOCK) OR otherwise most often the client disconnected
                    if (nbytes < 0) {
                        switch (errno) {
                            case EWOULDBLOCK: break;// Is expected because read is blocking
                            case ECONNRESET:
                                // Client on socket i reset connection, remove from list
                                for (int j=0; j < MAXCLIENTS; j++) {
                                    if (cliaddresses[j].socknum == i) {
                                        // Make space for new entries
                                        cliaddresses[j].occupied = false;
                                        // Is this the highest fd number?
                                        if (i == maxfd) {
                                            // set maxfd to highest fd integral we use anywhere
                                            int tmaxfd = s_tcp;
                                            for (int z=0; z < MAXCLIENTS; z++) {
                                                if (cliaddresses[z].occupied && cliaddresses[z].socknum > tmaxfd && z != i) {
                                                    tmaxfd = cliaddresses[z].socknum;
                                                }
                                            }
                                            maxfd = tmaxfd;
                                        }
                                        // Close socket
                                        close(cliaddresses[j].socknum);
                                        // Clear from fd set
                                        FD_CLR(i, &master);
                                        printf("Client on sockfd %d disconnected", i);
                                    }
                                } break;
                            default:
                                perror("Error different than expected EWOULDBLOCK errno occured");
                                fflush(stderr);
                        }
                    }

                    if(strcmp(buffer, "Put") == 0) {
                        printf("Received put request from Client\n");
                        //Receive file from Client
                        nbytes = receiveFileFromClient(i);
                        if(nbytes < 0)
                        {
                            perror("Error during the read function...");
                            fflush(stderr);
                        }
                        //Save received Client file on disk
                        nbytes = saveClientFile(nbytes, buffer);
                        if(nbytes < 0)
                        {
                            perror("Error while writing function");
                            fflush(stderr);
                        }
                    }

                    if(strcmp(buffer, "Get") == 0) {
                        printf("Received get request from Client\n");
                        nbytes = read(i, buffer, MAXDATASIZE);
                        if(nbytes < 0)
                        {
                            perror("Error during the read function...");
                            fflush(stderr);
                        }
                        nbytes = sendFile(buffer, i);
                        if(nbytes < 0)
                        {
                            perror("Error during the sendFile function...");
                            fflush(stderr);
                        }
                        break;
                    }
                    if(strcmp(buffer, "List") == 0) {
                        printf("Received List request from a Client\n");
                        printf("Bufferinhalt in Listzweig nach Read-Aufruf: %s\n", buffer); 
                        
                        char listinfo[MAXDATASIZE] = {0};
                        
                        // Add client informations
                        strcat(listinfo, "Clients connected to Server:\n");
                        char hostname[50];
                        char port[10];
                        for (int j=0; j < MAXCLIENTS; j++) {
                            if (cliaddresses[j].occupied) {
                                cliaddrp = &cliaddresses[j].cliaddr;
                                struct sockaddr* q = (struct sockaddr*)  cliaddrp;
                                
                                // Get client port directly from struct and calculate sockaddr size
                                int hostaddrlen;
                                if(q->sa_family == AF_INET) {
                                    // Read client port
                                    sprintf(port, "%d", ((struct sockaddr_in*)(q))->sin_port);
                                    // Calc sockaddr size
                                    hostaddrlen = sizeof(struct sockaddr_in);
                                } else {    // AF_INET6
                                    // Read client port
                                    sprintf(port, "%d", ((struct sockaddr_in6*)(q))->sin6_port);
                                    // Calc sockaddr size
                                    hostaddrlen = sizeof(struct sockaddr_in6);
                                }
                                
                                int status = 0;
                                status = getnameinfo(q, hostaddrlen, hostname, sizeof(hostname), NULL, 0, 0);
                                if (0 != status) {
                                    fprintf(stderr, "Error: %s\n", gai_strerror(status)); // gai_strerror is getaddrinfo's Error descriptor
                                    fflush(stderr);
                                }
                                
                                strcat(listinfo, hostname);
                                strcat(listinfo, ":");
                                strcat(listinfo, port);
                                strcat(listinfo, "\n");
                            }
                        }
                        
                        // Add file informations
                        strcat(listinfo, "\nFiles in Server directory:\n");
                        DIR *d;
                        struct dirent *dir;
                        d = opendir(".");
                        if (NULL == d) {
                            perror("Error reading directory");
                            fflush(stderr);
                        } else {
                            // Add each file to the info
                            while ((dir = readdir(d)) != NULL)
                            {
                                strcat(listinfo, dir->d_name);
                                strcat(listinfo, "\n");
                            }
                            closedir(d);
                        }
                        
                        printf("%s", listinfo); 
                        // Send informations to client
                        write(i, listinfo, sizeof(listinfo));
                        break;
                    }
                    //printf("\n%i bytes received.\n", nbytes);
                    fflush(stdout);
                    bzero(buffer, MAXDATASIZE);
                }
            }
        }
    }
}


/*
 * Receiving a file from a Client
 */
int receiveFileFromClient(int sock){
    int nbytes;
    memset(buffer, 0, MAXDATASIZE);
    memset(filenamePut, 0, MAXDATASIZE);	
    //readfilename
    nbytes = read(sock, filenamePut, MAXDATASIZE);
    if(nbytes < 0)
    {
        perror("Error during the read function...");
    }
    nbytes = read(sock, buffer, MAXDATASIZE); 
    if(nbytes < 0)
    {
        perror("Error during the read function...");
    }
    printf("Data received: %i bytes\n", nbytes);
    return nbytes;
}

/*
 * Save a received file from a Client on disk.
 */
int saveClientFile(int bytes, char* filename){
    //save received data on disk
    FILE* receivedFile;
    receivedFile = fopen(filenamePut, "wb");
    //write file on disk
    int n = fwrite(buffer, bytes, 1, receivedFile);
    if(n < 0){
	perror("Error during fwrite function");
    }
    //close file
    fclose(receivedFile);
    //clear the buffer
    memset(buffer, 0, MAXDATASIZE);
    return n;
}

/*
 * Helpfunction for getting Filecontent
 */
int getFileContent(char* filename, char* content){
    FILE* fp = fopen(filename, "rb");
    int size = 0;
    char buffer[MAXDATASIZE];
    size_t i;
    for (i = 0; i < MAXDATASIZE; ++i)
    {
        int c = getc(fp);
        size++;
        if (c == EOF)
        {
            buffer[i] = 0x00;
            break;
        }
        buffer[i] = c;
    }
    fclose(fp);
    memcpy(content, buffer, size);
    return size;
}

/*
 * Send a requested File to a Client
 */
int sendFile(char* filename, int sock) {
    printf("Sending requested file to client\n");	
    char filecontent[MAXDATASIZE] = {0};
    int size = getFileContent(filename, filecontent);
    if(size < 0){
	perror("Error during getFileContent function");
        fflush(stdout);
    }
    int sentbytes = write(sock, filecontent, size);
    if(sentbytes < 0){
	perror("Error during write function");
        fflush(stdout);
    }
    printf("%i from %i bytes of data were sent\n", sentbytes, size);
    return sentbytes;
}
