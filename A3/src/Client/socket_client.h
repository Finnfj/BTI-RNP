#ifndef SOCKET_CLIENT_H_   /* Include guard */
#define SOCKET_CLIENT_H_


bool isValidArgument(char*);
int connectToServer(char*);
void disconnect();
int sendFile(char*);
int getFile(char*);
int readFileContent(char*, char*);
int sendRequest(char*);
int saveFile(int, char*);
int writeSocket(char*);
int readSocket();

#endif
