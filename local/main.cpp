#include "../WARDuino.h"
#include <iostream>

unsigned char hello_world_wasm[] = {
  0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x02, 0x60,
  0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x60, 0x00, 0x00, 0x02, 0x11, 0x01, 0x07,
  0x65, 0x73, 0x70, 0x38, 0x32, 0x36, 0x36, 0x05, 0x62, 0x6c, 0x69, 0x6e,
  0x6b, 0x00, 0x01, 0x03, 0x02, 0x01, 0x00, 0x05, 0x06, 0x01, 0x01, 0x80,
  0x02, 0x80, 0x02, 0x07, 0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x01,
  0x0a, 0x09, 0x01, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b
};
unsigned int hello_world_wasm_len = 71;

int main(int argc, const char * argv[]) {
    WARDuino* w = new WARDuino();
    w->run_module(hello_world_wasm,hello_world_wasm_len);
    return 0;
}

