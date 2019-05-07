#ifndef SOCKET_H_   /* Include guard */
#define SOCKET_H_

void connectToPeer(char*);
void disconnectFromPeer(void);
void sayToPeer(char*);
void sendToPeer(char*);
void hostConnection();
void quit();

#endif
