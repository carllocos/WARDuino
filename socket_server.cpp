#include "socket_server.h"

#ifdef ARDUINO
#include <WiFi.h>
#define MAX_SOCKETS 3

#include "SocketServer.h"

struct ClientSocket {
  unsigned short int which;
};

struct ClientSocket sockets[MAX_SOCKETS];
SocketServer *_socketServer;

AsyncClient* getClient(ClientSocket * client){
  switch(client->which){
    case 0:
      return _socketServer->forIO;
    case 1:
      return _socketServer->forEvents;
    case 2:
      return _socketServer->forProxy;
    default:
      printf("Requested incorrect Socket %hu. Giving regular IO\n", client->which);
      return _socketServer->forIO;
  }
}

struct ClientSocket* getOutputSocket() {
  if(_socketServer->forIO == nullptr)
    return nullptr;
  return sockets;
}

struct ClientSocket* getEventSocket() {
  if(_socketServer->forEvents == nullptr)
    return nullptr;
  return sockets + 1;
}

struct ClientSocket* getProxyOutput() {
    if(_socketServer->forProxy != nullptr)
      return sockets + 2;

    if(_socketServer->forEvents == nullptr) //TODO remove
        return getOutputSocket();

    return nullptr;
}


void write2Client(struct ClientSocket* client, const void* buf, int count) {
    if (client == nullptr) return;
    AsyncClient* c = getClient(client);
    const char* cbuf = (char*)buf;
    size_t size_count = (size_t) count;
    size_t space_left = c->space();
    c->add(cbuf, size_count  > space_left ? space_left : size_count);
    c->send();
    if(size_count <= space_left){
      return;
    }
    /* while(!c->canSend()){ */
    /*   printf("write2Client: looping cannot send to client yet\n"); */
    /* } */
    write2Client(client, (const void *) (cbuf + space_left), (int) (size_count - space_left));
}

void flush2Client(struct ClientSocket* client) {
    if (client == nullptr) return;
    AsyncClient* c = getClient(client);
    while(!c->canSend()){
      printf("flush2Client: looping cannot send to client yet\n");
    }
    c->send();
}

char* getReceivedData() { return nullptr; }

uint32_t receivedDataSize() { return 0;}

void freeReceivedData() {return;}

void initializeServer(const char* host, int portno, const char* ssid,
                      const char* password){
  initializeServer(host, portno, ssid, password, nullptr);}

void initializeServer(const char* host, int portno, const char* ssid,
                      const char* password, WARDuino * wrd) {

    printf("Connecting to WiFi..\n\n");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
       delay(10);
    }

    printf("%d.%d.%d.%d\n\n", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    uint16_t port = (uint16_t) portno;
    _socketServer = new SocketServer(port, wrd);
    _socketServer->begin();
    sockets[0].which = 0;
    sockets[1].which = 1;
    sockets[2].which = 2;
}

void processIncomingEvents(){return; }

void socket_debug(const char* format, ...) {}

#else

#include <arpa/inet.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>

// temporary imports
#include <errno.h>   //TODO remove
#include <stdarg.h>  //TODO remove
#include <stdlib.h>  //TODO remove

#include <cstdio>  //TODO remove

#define MAX_SOCKETS 2

/*
private functions
*/
void _fillfds();
void _errorexit(const char *);
void _closeSocket(int);
void _acceptSockets();
bool _readInput();

struct ServerSocket {
    int fd;
    struct sockaddr_in *addr;
} server;

struct InputSocket {
    char *buff;
    uint32_t size;
} inputsocket;

struct ClientSocket {
    int fd;
};

struct ClientSocket sockets[MAX_SOCKETS];

struct pollfd fds[MAX_SOCKETS];

char *getReceivedData() {
    if (inputsocket.size > 0)
        return inputsocket.buff;
    else
        return nullptr;
}

uint32_t receivedDataSize() {
    if (inputsocket.size > 0)
        return inputsocket.size;
    else
        return 0;
}

void freeReceivedData() { inputsocket.size = 0; }

struct ClientSocket *getOutputSocket() {
    if (sockets[0].fd != -1) return sockets;
    return nullptr;
}

struct ClientSocket *getEventSocket() {
    if (sockets[1].fd != -1) return sockets + 1;
    return nullptr;
}

struct ClientSocket *getProxyOutput() {
    return nullptr;
}

void write2Client(struct ClientSocket *client, const void *buff, int count) {
    write(client->fd, buff, count);
}

void flush2Client(struct ClientSocket *client) { return; }


void initializeServer(const char *host, int portno, const char *ssid,
                      const char *password) {
  initializeServer(host, portno, ssid, password, nullptr);
}

void initializeServer(const char *host, int portno, const char *ssid,
                      const char *password, WARDuino *wrd) {
    printf("inializing socket at host %s - port %d\n", host, portno);

    int listen_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sd < 0) _errorexit("socket()\n");  // TODO use fatal?

    int enable = 1;
    if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, (char *)&enable,
                   sizeof(enable)) < 0)
        _errorexit("setsockopt()\n");

    if (ioctl(listen_sd, FIONBIO, (char *)&enable) < 0)
        _errorexit("error on ioctl()\n");

    server.addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    bzero((char *)server.addr, sizeof(*(server.addr)));

    server.addr->sin_family = AF_INET;
    server.addr->sin_addr.s_addr = INADDR_ANY;
    server.addr->sin_port = htons(portno);

    if (bind(listen_sd, (struct sockaddr *)server.addr,
             sizeof(*(server.addr))) < 0)
        _errorexit("error on bind()\n");

    if (listen(listen_sd, SOCK_BACKLOG_SIZE) < 0)
        _errorexit("error on listen()\n");

    // init state
    server.fd = listen_sd;
    inputsocket.buff = (char *)malloc(SOCK_RECVBUF_SIZE);
    for (auto i = 0; i < MAX_SOCKETS; i++) sockets[i].fd = -1;
}

void processIncomingEvents() {
    _fillfds();

    int timeout = 5;  // ms
    int qevs = poll(fds, MAX_SOCKETS, timeout);
    if (qevs < 0) _errorexit("poll()\n");
    if (qevs == 0)  // timeout
        return;

    for (auto i = 0; i < MAX_SOCKETS; i++) {
        if (fds[i].revents == 0) continue;

        if (fds[i].revents != POLLIN) {
            _closeSocket(fds[i].fd);
            continue;
        }

        if (fds[i].fd == server.fd) {
            _acceptSockets();
        } else if (fds[i].fd == sockets[0].fd) {
            /* input socket is allowed to send data.
               i.e. the interrupt data consumed by WARDuino
            */
            if (!_readInput()) {
                _closeSocket(sockets[0].fd);
                printf("closing input socket fd=%d\n", fds[0].fd);
            }
        } else {
            _closeSocket(fds[i].fd);
        }
    }
}

bool _readInput() {
    // printf("receiving input\n");
    int len = recv(sockets[0].fd, inputsocket.buff, SOCK_RECVBUF_SIZE, 0);
    if (len > 0) {
        inputsocket.size = len;
        // printf("received total #%" PRIu32 "\n", inputsocket.size);
        return true;
    }
    return false;
}

void _acceptSockets() {
    int sd = -1;
    do {
        sd = accept(server.fd, 0, 0);
        if (sd < 0) {
            if (errno != EWOULDBLOCK) {
                printf("WARNING CLOSING SOCKET SERVER fd=%d\n", server.fd);
                _closeSocket(server.fd);
            }
            break;
        }

        int pos = -1;
        if (sockets[0].fd == -1) {
            pos = 0;
        } else if (sockets[1].fd == -1) {
            pos = 1;
        }

        printf("accepting socket %d at %d \n", sd, pos);
        if (pos == -1) {
            close(sd);
        } else {
            sockets[pos].fd = sd;
        }
    } while (sd != -1);
}

void _closeSocket(int fd) {
    close(fd);
    if (server.fd == fd) {
    } else {
        int p = sockets[0].fd == fd ? 0 : 1;
        sockets[p].fd = -1;
        printf("closing socket fd=%d at %d\n", fd, p);
    }
}

void _fillfds() {
    fds[0].fd = server.fd;
    fds[1].fd = sockets[0].fd;
    fds[2].fd = sockets[1].fd;
    for (auto i = 0; i <= MAX_SOCKETS; i++) fds[i].events = POLLIN;
}

void _errorexit(const char *format) {
    perror(format);
    exit(-1);
}

#endif
