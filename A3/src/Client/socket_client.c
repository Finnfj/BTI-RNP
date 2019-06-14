/**Socket Client Application [RNP Aufgabe 3 Prof. Dr. Schmidt]
 * -----------------------------------
 * Authors: Finn Jannsen & Paul Mathia 
 * Version: 1.0
 * Description:
 * This is a simple C socket client able to send and get files to/from a corresponding server.
 * It supports both IPv4/6 connections.
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "socket_client.h"

#define SRV_PORT    "4715"
#define MAXDATASIZE 1000
#define PUT_C       "Put"
#define GET_C       "Get"
#define LIST_C      "List"

// Global values
bool connected = false;
int sockfd;
char buffer[MAXDATASIZE];  

/**
 * Main loop. Read input and act accordingly
 */
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
        //getfirst token and the delimiter is: " "
        nextToken = strtok(linebuf, " ");

        // Check what command was entered
        if(strcmp(linebuf, "Connect") == 0 || strcmp(linebuf, "connect") == 0) {
            // Connect if we aren't, do nothing otherwise
            if (connected) {
                fprintf(stderr, "We are already connected\n");
                fflush(stderr);
            } 
            else {
                // Get second argument (address)
                nextToken = strtok(NULL, " ");
                if (isValidArgument(nextToken)) {
                    // Try connecting to server
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
                    fprintf(stderr, "Argument missing\n\n");
                    fflush(stderr);
                }
            }
        } 
	    else if(strcmp(linebuf, "List") == 0 || strcmp(linebuf, "list") == 0) {
            // set list request to server
            sendRequest(LIST_C);
            
            // read response
            if (-1 == readSocket()) {
                fprintf(stderr, "Couldn't read 'List' response\n");
                fflush(stderr);
            } else {
                printf("\n%s", buffer); 
            }
        }
        else if(strcmp(linebuf, "Put") == 0 || strcmp(linebuf, "put") == 0) {
            // set put information to server
            sendRequest(PUT_C);
            
            // Get second argument (filename)
            nextToken = strtok(NULL, " ");
            if(isValidArgument(nextToken)) {
                //sending a file to server
                sendFile(nextToken);
            } else {
                fprintf(stderr, "Argument missing\n\n");
                fflush(stderr);
            }
        }
        else if (strcmp(linebuf, "Get") == 0 || strcmp(linebuf, "get") == 0){
            //send get information to server
            sendRequest(GET_C);
            
            // Get second argument (filename)
            nextToken = strtok(NULL, " ");
            if(isValidArgument(nextToken)) {
                //getting file from server
                getFile(nextToken);
            } else {
                fprintf(stderr, "Argument missing\n\n");
                fflush(stderr);
            }
        } else if (strcmp(linebuf, "Quit") == 0 || strcmp(linebuf, "quit") == 0) {
            // Disconnect and quit program
            disconnect();
            exit(EXIT_SUCCESS);
        } else {
            puts("Unknown command\n\n");
            fflush(stdout);
        }
        free(linebuf);
        linebuf = NULL;
    }
}


/**
 * Check if argument is not null or length 0
 * @param string to check
 * @return true means valid string, false invalid
 */
bool isValidArgument(char* str)
{
    bool valid = (str != NULL) && (strlen(str) > 0);
    return valid;
}


/**
 * Send a file to server
 * @param filename name of the file. It has to be in the program directory
 */
int sendFile(char* filename) {
    printf("Sending %s to Server\n", filename);
    char filebuf[MAXDATASIZE] = {0};
    
    // add filesize as header
    struct stat attributes;
    if(stat(filename, &attributes) != 0) {
        fprintf(stderr, "There was an error accessing '%s' or it doesn't exist.\n", filename);
        fflush(stderr);
        return -1;
    }
    sprintf(filebuf, "%ld!%s", attributes.st_size, filename);
    
    // send file metadata to server
    if (-1 == writeSocket(filebuf)) {
        fprintf(stderr, "Couldn't write file metadata to socket\n");
        fflush(stderr);
        return -1;
    }
    
    // open file
    int size = readFileContent(filename, filebuf, MAXDATASIZE);
    
    // send filecontent to server
    if (-1 == writeSocket(filebuf)) {
        fprintf(stderr, "Couldn't write filecontent to socket\n");
        fflush(stderr);
        return -1;
    }
    printf("%i bytes of data were sent to the Server\n", size);
    
    // get server response
    int nbytes = readSocket();
    if (-1 == nbytes) {
        fprintf(stderr, "Couldn't read file information from socket\n");
        fflush(stderr);
        return -1;
    }
    printf("%s\n", buffer);
    fflush(stdout);
    return 0;
}

/**
 * Get a file from server
 * @param filename name of the file. It has to be in the server directory
 */
int getFile(char* filename) {
    printf("Requesting following File: %s from Server\n", filename);
    fflush(stdout);
    
    //send filename to server
    if (-1 == writeSocket(filename)) {
        fprintf(stderr, "Couldn't write filename to socket\n");
        fflush(stderr);
        return -1;
    }
    
    // read file information
    int nbytes = readSocket();
    if (-1 == nbytes) {
        fprintf(stderr, "Couldn't read file information from socket\n");
        fflush(stderr);
        return -1;
    }
    // take out header (size)
    char* ptr;
    ptr = strtok(buffer, "!");
    int size;
    size = atoi(ptr);
    // print information
    ptr = strtok(NULL, "!");
    printf("\n%s\n", ptr);
    
    // read file
    nbytes = readSocket();
    if (-1 == nbytes) {
        fprintf(stderr, "Couldn't read file from socket\n");
        fflush(stderr);
        return -1;
    }
    
    // display content
    printf("\nData received: %i bytes\n", size);
    printf("\nContent of %s = \n%s\n", filename, buffer);
    
    // save file
    if (-1 == saveFile(size, filename)) {
        fprintf(stderr, "Couldn't save file\n");
        fflush(stderr);
        return -1;
    }
    
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
 * Send a request to the server
 * @param ch request to send
 * @return 0 on success. -1 on failure
 */
int sendRequest(char* ch)
{
    char* flag = ch;
    printf("\nClient sending following request to the server: %s\n", flag);
    int nbytes = send(sockfd, ch, MAXDATASIZE, 0);
    if(nbytes < 0)
    {
        perror("Error sending request to Server..."); 
        fflush(stderr);
        return -1;
    }
    
    return 0;
}

/**
 * Save a requested file from Server on disk.
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
 * Save a requested file from Server on disk.
 * @param pl buffer from which is written
 * @return 0 on success. -1 on failure
 */
int writeSocket(char* pl) {
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
 * @return 0 on success. -1 on failure
 */
int readSocket() {
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
 * Connect to a server with the given address
 * @param address IP or hostname of the server socket to connect to
 * @return server socket fd or error code -1
 */
int connectToServer(char* address) {
    int s_tcp;			 // socket descriptor
    struct addrinfo hints;       // getaddrinfo parameters
    struct addrinfo* res;        // getaddrinfo result
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
        inet_ntop(AF_INET6, peerAddress, addrstrbuf, sizeof(addrstrbuf));
        printf("Connected to %s.\n", addrstrbuf);
        fflush(stdout);
    }
    // Connection complete, free addrinfo memory
    freeaddrinfo(res);
    return s_tcp;
}

/**
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
