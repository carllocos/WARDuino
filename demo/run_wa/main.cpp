#include <csignal>
#include <iostream>

#ifndef SOCKET
#define SOCKET 1
#endif

#if SOCKET
#include "../../WARDuino.h"
#include "../../socket_server.h"
#include "wa_sources/hello_world.c"

//decr=2
unsigned char countdown_wasm[] = {
  0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0d, 0x03, 0x60,
  0x01, 0x7e, 0x00, 0x60, 0x01, 0x7e, 0x01, 0x7e, 0x60, 0x00, 0x00, 0x03,
  0x04, 0x03, 0x00, 0x01, 0x02, 0x04, 0x05, 0x01, 0x70, 0x01, 0x02, 0x02,
  0x05, 0x03, 0x01, 0x00, 0x02, 0x06, 0x0b, 0x02, 0x7f, 0x00, 0x41, 0x00,
  0x0b, 0x7f, 0x01, 0x41, 0x00, 0x0b, 0x07, 0x08, 0x01, 0x04, 0x6d, 0x61,
  0x69, 0x6e, 0x00, 0x02, 0x09, 0x08, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x02,
  0x01, 0x00, 0x0a, 0x27, 0x03, 0x02, 0x00, 0x0b, 0x14, 0x00, 0x20, 0x00,
  0x42, 0x00, 0x55, 0x04, 0x7e, 0x20, 0x00, 0x42, 0x01, 0x7d, 0x10, 0x01,
  0x05, 0x42, 0x00, 0x0b, 0x0b, 0x0d, 0x00, 0x03, 0x40, 0x42, 0x02, 0x10,
  0x01, 0x10, 0x00, 0x0c, 0x00, 0x0b, 0x0b
};
unsigned int countdown_wasm_len = 115;


WARDuino wac;

int main(int /*argc*/, const char ** /*argv*/) {
    int portno = 8080;
    const char *host = "localhost";
    initializeServer(host, portno, nullptr, nullptr);
    RmvModule * rm = wac.removable(
        wac.load_module(countdown_wasm, countdown_wasm_len, {}));

    //wac.initial_runstate = WARDUINOpause;
    wac.run_module(rm);
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
    wac.run_module(wac.removable(
        wac.load_module(hello_world_wasm, hello_world_wasm_len, {})));
    return 0;
}
#endif