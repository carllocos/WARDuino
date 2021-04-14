
#if defined(Arduino)
bool proxy_connect(const char* host, int portno) { return false; }
void proxy_send(void* buffer, int size) { return; };
#else

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void error(const char *msg) {
    perror(msg);
    exit(0);
}

int sockfd, portno, n;
struct sockaddr_in aserv_addr;
struct hostent *aServer;

bool proxy_connect(const char *host, int portno) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");
    printf("sockfd open %d\n", sockfd);
    aServer = gethostbyname(host);
    if (aServer == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&aserv_addr, sizeof(aserv_addr));
    aserv_addr.sin_family = AF_INET;
    bcopy((char *)aServer->h_addr, (char *)&aserv_addr.sin_addr.s_addr,
          aServer->h_length);
    aserv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&aserv_addr, sizeof(aserv_addr)) < 0)
        error("ERROR connecting");
    printf("done connecting\n");
    return true;
}

char *proxy_send(void *buffer, int size) {
    printf("sending\n");
    n = write(sockfd, buffer, size);
    if (n < 0) error("ERROR writing to socket");
    char *b = (char *)malloc(sizeof(char) * 50);
    bzero(b, 50);
    printf("about to read. send total %d\n", n);
    n = read(sockfd, b, 50);
    if (n < 0) error("ERROR reading from socket");

    return b;
}

#endif
