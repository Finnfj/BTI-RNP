#ifndef SOCKET_CLIENT_H_   /* Include guard */
#define SOCKET_CLIENT_H_


bool isValidArgument(char*);
int connectToServer(char*);
void disconnect();
void sendFile(char*);
void getFile(char*);
int getFileContent(char*, char*);
#endif