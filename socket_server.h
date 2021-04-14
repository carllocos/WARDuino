#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include <inttypes.h>

#define SOCK_SENDBUF_SIZE 1024
#define SOCK_RECVBUF_SIZE 1024
#define SOCK_BACKLOG_SIZE 5

struct ClientSocket;

struct ClientSocket* getOutputSocket();

struct ClientSocket* getEventSocket();

struct ClientSocket* getProxyOutput();

// TODO replace send2Client with write2Clien
void send2Client(struct ClientSocket* client, char* buffer, int size);

void write2Client(struct ClientSocket* client, const void* buff, int count);

void flush2Client(struct ClientSocket* client);
/*
initializes socket server at host and port number.
*/
void initializeServer(const char* host, int portno, const char* ssid,
                      const char* password);

void processIncomingEvents();

/*
returns a pointer to char if data has been read by a FLAG_IN configured socket.
If no data has been read a nullptr is returned
*/
char* getReceivedData();

/*
returns the size of getReceivedData();
*/
uint32_t receivedDataSize();

/*
frees the received data
*/
void freeReceivedData();

#endif