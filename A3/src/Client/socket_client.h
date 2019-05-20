#ifndef SOCKET_CLIENT_H_   /* Include guard */
#define SOCKET_CLIENT_H_


bool isValidArgument(char*);
int connectToServer(char*);
void disconnect();
void sendFile(char*);
int getFileFromServer(char*);
int getFileContent(char*, char*);
int sendRequest(char*);
int saveServerFile(int , char*, char*);

#endif
