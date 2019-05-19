#ifndef SOCKET_SRV_H_   /* Include guard */
#define SOCKET_SRV_H_


void free_sock();
int receiveFileFromClient(int);
int saveClientFile(int bytes);
int getFileContent(char*, char*);
int sendFile(char*, int);

#endif
