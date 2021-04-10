#include <csignal>
#include <iostream>

#ifndef SOCKET
#define SOCKET 0
#endif

#if SOCKET
#include "../../WARDuino.h"
#include "../../socket_server.h"
#include "wa_sources/hello_world.c"

WARDuino wac;

int main(int /*argc*/, const char ** /*argv*/) {
    int portno = 8080;
    const char *host = "localhost";
    initializeServer(host, portno, nullptr, nullptr);
    wac.run_module(wac.removable(wac.load_module(hello_world_wasm, hello_world_wasm_len, {})));
    return 0;
}

#else
#include "../../WARDuino.h"

extern "C" {
// TODO: Stat files, alternative needed for arduino
#include <sys/stat.h>
// END
}
#include "wa_sources/hello_world.c"

WARDuino wac;

volatile bool handlingInterrupt = false;
void signalHandler(int /* signum */) {
    if (handlingInterrupt) return;

    printf("CHANGE REQUESTED!");
    struct stat statbuff {};
    if (stat("/tmp/change", &statbuff) == 0 && statbuff.st_size > 0) {
        auto *data = (uint8_t *)malloc(statbuff.st_size * sizeof(uint8_t));
        FILE *fp = fopen("/tmp/change", "rb");
        fread(data, statbuff.st_size, 1, fp);
        fclose(fp);
        wac.handleInterrupt(statbuff.st_size, data);
    }

    handlingInterrupt = false;
}

/**
 * Run code, ececute interrups in /tmp/change if a USR1 signal comes
 */

int main(int /*argc*/, const char ** /*argv*/) {
    signal(SIGUSR1, signalHandler);
    wac.run_module(wac.removable( wac.load_module(hello_world_wasm, hello_world_wasm_len, {})));
    return 0;
}
#endif