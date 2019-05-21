#ifndef SOCKET_SRV_H_   /* Include guard */
#define SOCKET_SRV_H_


void free_sock();
int receiveFileFromClient(int);
int saveFile(int, char*);
int readFileContent(char*, char*);
int sendFile(char*, int);
void removeClient(int);
int writeSocket(char*, int);
int readSocket(int);
int listInformations(char*);

#endif
