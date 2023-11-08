#pragma once

enum RunningState {
    WARDUINOinit = 0x01,
    WARDUINOrun = 0x02,
    WARDUINOpause = 0x03,
    WARDUINOstep = 0x04,
    PROXYrun = 0x05,  // Running state used when executing a proxy call. During
    // this state the call is set up and executed by the main
    // loop. After execution, the state is restored to
    // PROXYhalt
    PROXYhalt = 0x06  // Do not run the program (program runs on computer, which
    // sends messages for primitives, do forward interrupts)
};