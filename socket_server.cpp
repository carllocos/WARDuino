#include "socket_server.h"

#if defined(Arduino)
#include <WiFi.h>
#define MAX_SOCKETS 3

struct InputSocket {
    char* buff;
    const uint32_t max_size = SOCK_RECVBUF_SIZE;
    uint32_t size;
} inputsocket;

struct ClientSocket {
    WiFiClient socket;
    bool inuse;
};

struct ClientSocket sockets[MAX_SOCKETS];
short int nsockets = 0;

WiFiServer wifiServer(80);

struct ClientSocket* getClient(int i) {
    if (!sockets[i].inuse) return nullptr;

    struct ClientSocket c = sockets[i];
    if (c.socket.connected()) {
        return sockets + i;
    }
    return nullptr;
}

struct ClientSocket* getOutputSocket() {
    return getClient(0);
}

struct ClientSocket* getEventSocket() {
    return getClient(1);
}

struct ClientSocket* getProxyOutput() {
    if(nsockets == 1){ //TODO remove
        return getClient(0);
    }
    return getClient(2);
}

void send2Client(struct ClientSocket* client, char* buffer, int size) {
    if (client == nullptr) return;
    client->socket.write(buffer);
}

void write2Client(struct ClientSocket* client, const void* buf, int count) {
    if (client == nullptr) return;
    const char* cbuf = (char*)buf;
    uint32_t qw = client->socket.write(cbuf, count);

    // FIXME
    if (qw != count) Serial.println("writeClient not all send");
    client->socket.flush();
}

void flush2Client(struct ClientSocket* client) {
    if (client == nullptr) return;
    client->socket.flush();
}

char* getReceivedData() {
    if (inputsocket.size > 0) return inputsocket.buff;
    return nullptr;
}

uint32_t receivedDataSize() {
    if (inputsocket.size > 0) return inputsocket.size;
    return 0;
}

void freeReceivedData() { inputsocket.size = 0; }

void initializeServer(const char* host, int portno, const char* ssid,
                      const char* password) {
    WiFi.begin(ssid, password);

    Serial.println("Connecting to WiFi..");
    while (WiFi.status() != WL_CONNECTED) {
        delay(10);
    }
    Serial.println("Connected to the WiFi network");
    Serial.println(WiFi.localIP());

    wifiServer.begin();
    inputsocket.buff = (char*)malloc(inputsocket.max_size);
    sockets[0].inuse = false;
    sockets[1].inuse = false;
    sockets[2].inuse = false;
}

void processIncomingEvents() {
    // TODO use for loop
    if (nsockets < MAX_SOCKETS) {
        WiFiClient client = wifiServer.available();
        if (client) {
            short int idx = 2;
            if (!sockets[0].inuse) {
                idx = 0;
            } else if (!sockets[1].inuse) {
                idx = 1;
            }
            //printf("adding client %d\n", idx);
            sockets[idx].socket = client;
            sockets[idx].inuse = true;
            nsockets++;
        }
    }

    if(nsockets == 0){
        return;
    }

    if (sockets[1].inuse && !sockets[1].socket.connected()) {
        sockets[1].socket.stop();
        sockets[1].inuse = false;
        nsockets--;
        printf("stoping event client. nsockets %d\n", nsockets);
    }

    // todo use for loop!
    if (sockets[0].inuse && !sockets[0].socket.connected()) {
        sockets[0].socket.stop();
        sockets[0].inuse = false;
        nsockets--;
        printf("stoping inout client. nsockets %d\n", nsockets);
    } else if (sockets[0].inuse) {
        WiFiClient c = sockets[0].socket;
        uint32_t old_size = inputsocket.size;
        inputsocket.size = 0;
        while (c.available() > 0) {
            inputsocket.buff[inputsocket.size++] = c.read();
        }
        inputsocket.size = inputsocket.size == 0 ? old_size : inputsocket.size;
    }

    if (sockets[2].inuse && !sockets[2].socket.connected()) {
        sockets[2].socket.stop();
        sockets[2].inuse = false;
        nsockets--;
        printf("stoping proxy client. nsockets %d\n", nsockets);
    } else if (sockets[2].inuse) {
        WiFiClient c = sockets[2].socket;
        uint32_t old_size = inputsocket.size;
        inputsocket.size = 0;
        while (c.available() > 0) {
            inputsocket.buff[inputsocket.size++] = c.read();
        }
        inputsocket.size = inputsocket.size == 0 ? old_size : inputsocket.size;
    }
}

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

void send2Client(struct ClientSocket *client, char *buffer, int size) {
    write(client->fd, buffer, size);
}

void write2Client(struct ClientSocket *client, const void *buff, int count) {
    write(client->fd, buff, count);
}

void flush2Client(struct ClientSocket *client) { return; }

void initializeServer(const char *host, int portno, const char *ssid,
                      const char *password) {
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
    int qevs = poll(fds, (MAX_SOCKETS + 1), timeout);
    if (qevs < 0) _errorexit("poll()\n");
    if (qevs == 0)  // timeout
        return;

    for (auto i = 0; i <= MAX_SOCKETS; i++) {
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