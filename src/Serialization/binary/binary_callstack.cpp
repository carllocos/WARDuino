#include "binary_callstack.h"

#include "../../Utils/macros.h"
#include "../../Utils/util.h"
#include "binary_frame.h"
//#include "binary_multiple_objects.h"

void BINARY_serialize_Callstack(const Channel& channel, const Frame* callstack,
                                int begin, int end, const Module* m) {
    // format: callstack_kind | begin idx | end idx | frame 1 | frame 2 | ....


    size_t encoding_size = 1 + size_for_int(begin) + size_for_int(end);

}

FrameDeserialized BINARY_deserialize_Callstack(uint8_t* src) {
    FATAL("Callstack json deserialization not implemented\n");

}
