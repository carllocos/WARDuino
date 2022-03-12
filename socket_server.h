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

void write2Client(struct ClientSocket* client, const void* buff, int count);

void flush2Client(struct ClientSocket* client);
/*
initializes socket server at host and port number.
*/
class WARDuino;

void initializeServer(const char* host, int portno, const char* ssid,
                      const char* password);

void initializeServer(const char* host, int portno, const char* ssid,
                      const char* password, WARDuino * wrd);

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

void setConnectivityStatusPin(uint8_t pinNr);
void toggleWiFiConnection();
void showLedConnectivy();

bool isServerConnected();

#endif
