#ifndef PROXY_H
#define PROXY_H
bool proxy_connect(const char* host, int portno);
char* proxy_send(void* buffer, int size);
#endif