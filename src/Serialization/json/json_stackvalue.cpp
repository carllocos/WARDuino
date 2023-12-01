#include "../../Utils/macros.h"
#include "../../Utils/util.h"
#include "json_stackvalue.h"

void JSON_serialize_StackValue(const Channel& channel, const StackValue& value,
                               uint32_t value_idx) {
    char buff[256];
    switch (value.value_type) {
        case I32:
            snprintf(buff, 255, R"("type":"i32","value":%)" PRIi32,
                     value.value.uint32);
            break;
        case I64:
            snprintf(buff, 255, R"("type":"i64","value":%)" PRIi64,
                     value.value.uint64);
            break;
        case F32:
            snprintf(buff, 255, R"("type":"F32","value":"%)" PRIx32 "\"",
                     value.value.f32);
            break;
        case F64:
            snprintf(buff, 255, R"("type":"F64","value":"%a")",
                     value.value.f64);
            break;
        default:
            snprintf(buff, 255, R"("type":"%02x","value":"%)" PRIx64 "\"",
                     value.value_type, value.value.uint64);
    }
    channel.write(R"({"idx":%d,%s})", value_idx, buff);
}

void JSON_deserialize_StackValue(uint8_t* src, StackValue* dest_value) {
    FATAL("StackValue json deserialization not implemented\n");
}
