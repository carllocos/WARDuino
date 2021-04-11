#include "printing.h"

#include <stdarg.h>
#include <stdio.h>  //TODO might not work in esp

#if SOCKET

#include <stdlib.h>

#include "socket_server.h"

void _print2Socket(struct ClientSocket* client, const char* format,
                   va_list args);

void wa_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    _print2Socket(getOutputSocket(), format, args);
    va_end(args);
}
void wa_evprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    _print2Socket(getEventSocket(), format, args);
    va_end(args);
    flush2Client(getEventSocket());
}

void wa_dbgprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void wa_flush() { flush2Client(getOutputSocket()); }

void wa_write(const void* buff, int count) {
    struct ClientSocket* client = getOutputSocket();
    if (client == nullptr) {
        for (auto i = 0; i < count; i++) printf((char*)(buff + i));
        fflush(stdout);
    } else
        write2Client(client, buff, count);
}

char buffer[SOCK_SENDBUF_SIZE];
void _print2Socket(struct ClientSocket* client, const char* format,
                   va_list args) {
    // FIXME cleanup MACROS?
    if (client == nullptr) {
        vprintf(format, args);
    } else {
        int l = vsnprintf(buffer, SOCK_SENDBUF_SIZE, format, args);
        if (l == SOCK_SENDBUF_SIZE) {
            printf("TOO MUCH\n");
            exit(-1);
        }
        write2Client(client, buffer, l);
    }
}

#else

#include <stdio.h>  //TODO might not work in esp

void wa_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void wa_evprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
void wa_dbgprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void wa_write(const void* buff, int count) {
    const char* b = (char*)buff;
    for (auto i = 0; i < count; i++) printf((char*)b + i);
    fflush(stdout);
}
void wa_flush() { fflush(stdout); }

#endif