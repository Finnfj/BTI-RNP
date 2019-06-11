/**Socket Server Application [RNP Aufgabe 3 Prof. Dr. Schmidt]
 * -----------------------------------
 * Authors: Finn Jannsen & Paul Mathia 
 * Version: 1.0
 * Description:
 * This is a simple C socket server able to send and get files to/from a corresponding client and handle up to 5 clients
 * It supports both IPv4/6 connections.
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
#include <time.h>
#include "socket_srv.h"

#include <sys/select.h>
#include <fcntl.h>

#define SRV_PORT    "4715"
#define MAXDATASIZE 1000
#define MAXCLIENTS  5
#define PUT_C       "Put"
#define GET_C       "Get"
#define LIST_C      "List"

// manage client addresses
struct sockaddr_ref {
    bool occupied;
    int socknum;
    struct sockaddr_storage cliaddr;
    char* hostname;
};

// Global values
int s_tcp, maxfd;					            // Main socket descriptor
struct sockaddr_ref cliaddresses[MAXCLIENTS];   // Store client informations
char buffer[MAXDATASIZE];   			        // buffer
char ipaddr[INET6_ADDRSTRLEN];                  // store ip address
fd_set master;                                  // Filedescriptor set for non-blocking multiplexing
struct sockaddr_storage *cliaddrp;              // Pointer to a cliaddr from cliaddresses


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

    fd_set readfds;             // temp fd set to work with

    socklen_t cliaddrlen;                           // Set later due to possible padding causing different lengths, means sizeof(cliaddr)

    memset(&cliaddresses, 0, sizeof(cliaddresses));
    memset(&hints, 0, sizeof(hints));

    // either IPv4/IPv6
    hints.ai_family = AF_INET6;
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
        // One successful socket is enough
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

    /**----------------initialization complete, start main loop-----------------*/
    // Main loop values
    int news, n;
    maxfd = s_tcp; // Need to always know the maximum integer value of all file descriptors, starts with hosting socket fd
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
                        if (-1 == (news = accept(s_tcp,(struct sockaddr*) cliaddrp, &cliaddrlen)))
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
                    int nbytes;
                    //read requestflag
                    nbytes = readSocket(i);
                    
                    // Is read succesful - If this block is entered it would either block (EWOULDBLOCK) OR otherwise most often the client disconnected
                    if (nbytes < 0) {
                        switch (errno) {
                            case EWOULDBLOCK: break;// Is expected because read is blocking
                            case ECONNRESET:
                                // Client on socket i reset connection, remove from list
                                removeClient(i);
                                break;
                            default:
                                perror("Error different than expected EWOULDBLOCK or ECONNRESET errno occured");
                                fflush(stderr);
                        }
                    }

                    if(strcmp(buffer, PUT_C) == 0) {
                        printf("Received put request from Client\n");
                        //Receive file from Client
                        nbytes = receiveFileFromClient(i);
                        if (-1 == nbytes) {
                            fprintf(stderr, "Couldn't receive file\n");
                            fflush(stderr);
                            break;
                        }
                    }

                    if(strcmp(buffer, GET_C) == 0) {
                        printf("Received get request from Client\n");
                        // read filename
                        nbytes = readSocket(i);
                        if (-1 == nbytes) {
                            fprintf(stderr, "Couldn't read filename from socket\n");
                            fflush(stderr);
                            break;
                        }
                        // store filename
                        char filename[nbytes];
                        strncpy(filename, buffer, nbytes);
                        
                        // send the file
                        nbytes = sendFile(filename, i);
                        if (-1 == nbytes) {
                            fprintf(stderr, "Couldn't send file\n");
                            fflush(stderr);
                            break;
                        }
                        break;
                    }
                    if(strcmp(buffer, LIST_C) == 0) {
                        printf("Received List request from a Client\n");
                        
                        char listinfo[MAXDATASIZE] = {0};
                        
                        // Gather information
                        if (-1 == listInformations(listinfo)) {
                            fprintf(stderr, "Couldn't gather 'list' informations\n");
                            fflush(stderr);
                            break;
                        }
                        
                        // Send information to client
                        writeSocket(listinfo, i);
                        break;
                    }
                }
            }
        }
    }
}


 /**
 * Receive file from client.
 * @param sockfd client sockfd
 * @return 0 on success. -1 on failure
 */
int receiveFileFromClient(int sockfd){
    int nbytes;
    
    // read file metadata
    nbytes = readSocket(sockfd);
    if (-1 == nbytes) {
        fprintf(stderr, "Couldn't read file metadata from socket\n");
        fflush(stderr);
        return -1;
    }
    // take out header (size)
    char* ptr;
    ptr = strtok(buffer, "!");
    int size;
    size = atoi(ptr);
    // store filename
    ptr = strtok(NULL, "!");
    char filename[strlen(ptr)];
    strcpy(filename, ptr);
    
    // read actual file
    nbytes = readSocket(sockfd);
    if (-1 == nbytes) {
        fprintf(stderr, "Couldn't read file from socket\n");
        fflush(stderr);
        return -1;
    }
    
    printf("Data received: %i bytes\n", size);
    
    // save file and respond to client
    time_t rawtime;
    time(&rawtime);
    char timestamp[36];
    strftime(timestamp, 36, "%d.%m.%Y %H:%M:%S", localtime(&rawtime));
    if (-1 == saveFile(size, filename)) {
        fprintf(stderr, "Couldn't save file\n");
        fflush(stderr);
        sprintf(buffer,"FAIL %s\n", timestamp); 
    } else {
        sprintf(buffer,"OK %s\n", timestamp); 
    }
    // send response to client
    if (-1 == writeSocket(buffer, sockfd)) {
        fprintf(stderr, "Couldn't write response to socket\n");
        fflush(stderr);
        return -1;
    }
    return 0;
}

/**
 * Save a file on disk.
 * @param bytes size of the file in buffer
 * @param filename name of the file it is saved to
 * @return 0 on success. -1 on failure
 */
int saveFile(int bytes, char* filename){
    // create file
    FILE* receivedFile;
    receivedFile = fopen(filename, "wb");
    
    // write file
    int n = fwrite(buffer, bytes, 1, receivedFile);
    if(n < 0){
        perror("Error writing file..."); 
        fflush(stderr);
        return -1;
    }
    
    //close file
    fclose(receivedFile);
    
    //clear the buffer
    memset(buffer, 0, MAXDATASIZE);
    return 0;
}

/**
 * Read filecontent into a buffer
 * @param filename name of the file. It has to be in the program directory
 * @param content buffer to put the filecontent into
 * @param size size of the buffer
 * @return size of the file
 */
int readFileContent(char* filename, char* buf, int size){
    memset(buf, 0, size);
    FILE* fp = fopen(filename, "rb");
    int i;
    int c;
    
    for (i = 0; i < size; ++i)
    {
        c = getc(fp);
        if (c == EOF)
        {
            buf[i] = 0x00;
            break;
        }
        buf[i] = c;
    }
    
    fclose(fp);
    return i;
}

/**
 * Send a file to client
 * @param filename name of the file. It has to be in the program directory
 * @param sockfd client sockfd
 */
int sendFile(char* filename, int sockfd) {
    printf("Sending %s to Client\n", filename);
    
    // print data attributes into buffer
    printdatainfo(filename);
    
    // send file information to client
    if (-1 == writeSocket(buffer, sockfd)) {
        fprintf(stderr, "Couldn't write file information to socket\n");
        fflush(stderr);
        return -1;
    }
    
    // open file
    char filecontent[MAXDATASIZE] = {0};
    int size = readFileContent(filename, filecontent, MAXDATASIZE);
    
    // send filecontent to client
    if (-1 == writeSocket(filecontent, sockfd)) {
        fprintf(stderr, "Couldn't write filecontent to socket\n");
        fflush(stderr);
        return -1;
    }
    
    printf("%i bytes of data were sent to the Client\n\n", size);
    fflush(stdout);
    return 0;
}

/**
 * Remove a disconnected client from our entries.
 * @param sockfd client sockfd
 */
void removeClient(int sockfd) {
    for (int j=0; j < MAXCLIENTS; j++) {
        if (cliaddresses[j].socknum == sockfd) {
            // Make space for new entries
            cliaddresses[j].occupied = false;
            // Is this the highest fd number?
            if (sockfd == maxfd) {
                // set maxfd to highest fd integral we use anywhere
                int tmaxfd = s_tcp;
                for (int z=0; z < MAXCLIENTS; z++) {
                    if (cliaddresses[z].occupied && cliaddresses[z].socknum > tmaxfd && z != sockfd) {
                        tmaxfd = cliaddresses[z].socknum;
                    }
                }
                maxfd = tmaxfd;
            }
            // Close socket
            close(cliaddresses[j].socknum);
            // Clear from fd set
            FD_CLR(sockfd, &master);
            printf("Client on sockfd %d disconnected", sockfd);
        }
    }
}


/**
 * Save a requested file from Server on disk.
 * @param pl buffer from which is written
 * @param sockfd client sockfd
 * @return 0 on success. -1 on failure
 */
int writeSocket(char* pl, int sockfd) {
    int sentbytes = write(sockfd, pl, MAXDATASIZE);
    if(sentbytes < 0){
        perror("Error during write()..."); 
        fflush(stderr);
        return -1;
    }
    return 0;
}

/**
 * Read from socket.
 * @param sockfd client sockfd
 * @return 0 on success. -1 on failure
 */
int readSocket(int sockfd) {
    // clear buffer
    memset(buffer, 0, MAXDATASIZE); 
    
    int nbytes = read(sockfd, buffer, MAXDATASIZE);
    if(nbytes < 0)
    {
        perror("Error during the read()...");
        fflush(stderr);
        return -1;
    } else {
        return nbytes;
    }
}

/**
 * Gather informations on clients and files.
 * @param buf buffer to put information into
 * @return 0 on success. -1 on failure
 */
int listInformations(char* buf) {
    // Add client informations
    strcat(buf, "Clients connected to Server:\n");
    char hostname[50];
    char port[10];
    for (int j=0; j < MAXCLIENTS; j++) {
        if (cliaddresses[j].occupied) {
            cliaddrp = &cliaddresses[j].cliaddr;
            struct sockaddr* q = (struct sockaddr*)  cliaddrp;
            
            // Get client port directly from struct and calculate sockaddr size
            int hostaddrlen;
            sprintf(port, "%d", ntohs(((struct sockaddr_in6*)(q))->sin6_port));
            hostaddrlen = sizeof(struct sockaddr_in6);
            
            int status = 0;
            status = getnameinfo(q, hostaddrlen, hostname, sizeof(hostname), NULL, 0, 0);
            if (0 != status) {
                fprintf(stderr, "Error: %s\n", gai_strerror(status)); // gai_strerror is getaddrinfo's Error descriptor
                fflush(stderr);
                return -1;
            }
            
            strcat(buf, hostname);
            strcat(buf, ":");
            strcat(buf, port);
            strcat(buf, "\n");
        }
    }
    
    // Add file informations
    strcat(buf, "\nFiles in Server directory:\n");
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (NULL == d) {
        perror("Error reading directory");
        fflush(stderr);
        return -1;
    } else {
        // Add each file to the info
        while ((dir = readdir(d)) != NULL)
        {
            strcat(buf, dir->d_name);
            strcat(buf, "\n");
        }
        closedir(d);
    }
    
    return 0;
}

/**
 * Prints a string into the buffer which contains following information of a file "path" in the current server directory:
 * size as header
 * last modified time of a file
 * size of a file
 *
 * @param path path of file
 * @return 0 on success. -1 on failure
 */
int printdatainfo(char* path)
{
    struct stat attributes;
    
    if(stat(path, &attributes) != 0) {
        fprintf(stderr, "There was an error accessing '%s' or it doesn't exist.\n", path);
        fflush(stderr);
        return -1;
    }
    else {
        // format file timestamp
        char timestamp[36];
        strftime(timestamp, 36, "%d.%m.%Y %H:%M:%S", localtime(&attributes.st_atime));
        // format string
        memset(buffer, 0, MAXDATASIZE); 
        sprintf(buffer,"%ld!File '%s' was last modified: %s\nSize of '%s': %ld Bytes\n", attributes.st_size, path, timestamp, path, attributes.st_size); 
    }
    
    return 0;
}