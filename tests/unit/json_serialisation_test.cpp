#include <iostream>

#include "../../src/Serialization/serialization_factory.h"
#include "../../src/Utils/util.h"
#include "example_code/fac/fac_wasm.h"
#include "gtest/gtest.h"
#include "shared/base_interruptfixture.h"
#include "shared/json_companion.h"

class JSONSerialization : public BaseFixture {
   protected:
    SerializationStrategy serialization{};

    JSONSerialization() : BaseFixture(fac_wasm, fac_wasm_len) {}
    void SetUp() override {
        BaseFixture::SetUp();
        Serialization_factory(serialization, JSON_Serialization);
    }

    void TearDown() override {
        BaseFixture::TearDown();
        CallbackHandler::clear_callbacks();
    }

    void serializeMainFunctionFrame() {
        int fun_id = this->moduleCompanion->getMainFunctionID();
        this->callstackBuilder->pushFunctionCall(fun_id);

        int frame_index = this->wasm_module->csp;
        Frame fun_frame = this->wasm_module->callstack[frame_index];

        this->serialization.serializeFrame(this->output->getChannel(),
                                           &fun_frame, frame_index,
                                           this->wasm_module);
    }

    std::string numberToHexString(uint32_t n) {
        std::stringstream ss;
        ss << "0x" << std::hex << n;
        std::string hexSize = ss.str();
        return hexSize;
    }
};

using PCJSONSerialization = JSONSerialization;
TEST_F(PCJSONSerialization, PCIsVirtualAddress) {
    Block* main_func = this->moduleCompanion->getMainFunction();
    this->callstackBuilder->pushFunctionCall(main_func->fidx);

    uint32_t expectedPC = this->moduleCompanion->getVirtualAddressPC();

    serialization.serializePC(output->getChannel(), this->wasm_module->pc_ptr,
                              this->wasm_module);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "PC serialization did not receive a valid JSON.\n");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsKey("pc"))
        << this->fullErrorMessage("JSON does not contain `pc` key");
    ASSERT_TRUE(companion.containsOnlyKeys({"pc"}))
        << this->fullErrorMessage("JSON contains more than 1 key");

    try {
        uint32_t pc = parsed["pc"];
        ASSERT_EQ(pc, expectedPC)
            << "PC serialization did not print the expected pc virtual address";
        return;
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << "printed pc value cannot be converted to uint32_t\n"
               << "Exception message: " << e.what();
    }
}

using BreakpointsJSONSerialization = JSONSerialization;
TEST_F(BreakpointsJSONSerialization, BreakpointsAreVirtualAddresses) {
    // Creating breakpoints that need to be serialized
    std::set<uint32_t> virtualBPs = {0, 1, 2, 3};
    std::set<uint8_t*> physicalBPs = {};
    for (auto i = virtualBPs.begin(); i != virtualBPs.end(); ++i) {
        physicalBPs.insert(toPhysicalAddress(*i, this->wasm_module));
    }

    // do serialization to channel
    serialization.serializeBreakpoints(output->getChannel(), physicalBPs,
                                       this->wasm_module);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Breakpoints serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsKey("breakpoints"))
        << this->fullErrorMessage("JSON does not contain `breakpoints` key.");
    ASSERT_TRUE(companion.containsOnlyKeys({"breakpoints"}))
        << this->fullErrorMessage("JSON contains more than 1 key.");

    try {
        std::set<uint32_t> parsed_bps = parsed["breakpoints"];
        ASSERT_EQ(parsed_bps.size(), virtualBPs.size())
            << "expected #" << virtualBPs.size() << " breakpoints";
        for (auto bpIterator = virtualBPs.begin();
             bpIterator != virtualBPs.end(); bpIterator++) {
            ASSERT_TRUE(parsed_bps.count(*bpIterator))
                << "Breakpoint with virtual address " << *bpIterator
                << " is missing ";
        }
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << "printed breakpoints cannot be converted to a set of "
                  "uint32_t values\n"
               << "Exception message: " << e.what();
    }
}

using FrameJSONSerialization = JSONSerialization;
TEST_F(FrameJSONSerialization, FunctionFrameOnlyContainsExceptedKeys) {
    // push a Frame for the main function and serialize it to channel
    this->serializeMainFunctionFrame();

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    const std::unordered_set<std::string> expectedKeys{
        "type",      "fidx", "sp",    "fp",      "idx",
        "block_key", "ra",   "start", "callsite"};
    ASSERT_TRUE(companion.containsOnlyKeys(expectedKeys))
        << this->fullErrorMessage("JSON does not contain expected keys");
}

TEST_F(FrameJSONSerialization, FunctionFrameHasZeroType) {
    // push a Frame for the main function and serialize it to channel
    this->serializeMainFunctionFrame();

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        uint8_t block_type = parsed["type"];
        ASSERT_EQ(block_type, 0) << "Func frames should have zero type";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << "Frame `type` value cannot be converted to uint8_t\n"
               << "Exception message: " << e.what();
    }
}

TEST_F(FrameJSONSerialization, FunctionFrameHasFunctionID) {
    // push a Frame for the main function and serialize it to channel
    this->serializeMainFunctionFrame();

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        std::string fun_id_hex = parsed["fidx"];
        int main_id = this->moduleCompanion->getMainFunctionID();

        // Convert hex string to uint32_t
        uint32_t fun_id;
        std::stringstream ss;
        ss << std::hex << fun_id_hex;
        ss >> fun_id;

        ASSERT_EQ(fun_id, main_id)
            << "Func Frame does not have expected function id";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << " Frame `fidx` value cannot be converted to uint32_t\n"
               << "Exception message: " << e.what()
               << this->fullErrorMessage("");
    }
}

TEST_F(FrameJSONSerialization, FunctionFrameHasStackPointer) {
    int expected_frame_sp = 33;  // random
    this->wasm_module->sp = expected_frame_sp;

    // push a Frame for the main function and serialize it to channel
    this->serializeMainFunctionFrame();

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        int frame_sp = parsed["sp"];
        ASSERT_EQ(frame_sp, expected_frame_sp)
            << "Func Frame does not have expected stack pointer";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << " Frame `sp` value cannot be converted to int\n"
               << "Exception message: " << e.what()
               << this->fullErrorMessage("");
    }
}

TEST_F(FrameJSONSerialization, FunctionFrameHasFramePointer) {
    int expected_frame_pointer = 33;  // random value
    this->wasm_module->fp = expected_frame_pointer;

    // push a Frame for the main function and serialize it to channel
    this->serializeMainFunctionFrame();

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        int frame_fp = parsed["fp"];
        ASSERT_EQ(frame_fp, expected_frame_pointer)
            << "Func Frame does not have expected frame pointer";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << " Frame `fp` value cannot be converted to int\n"
               << "Exception message: " << e.what()
               << this->fullErrorMessage("");
    }
}

TEST_F(FrameJSONSerialization, FrameHasIndex) {
    Block block{};
    Frame frame{.block = &block, .ra_ptr = nullptr};

    int expected_frame_index = 55;  // random
    this->serialization.serializeFrame(this->output->getChannel(), &frame,
                                       expected_frame_index, this->wasm_module);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    try {
        int frame_index = parsed["idx"];
        ASSERT_EQ(frame_index, expected_frame_index)
            << "Func Frame does not have expected index";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << " Frame `idx` value cannot be converted to int\n"
               << "Exception message: " << e.what()
               << this->fullErrorMessage("");
    }
}

TEST_F(FrameJSONSerialization, FunctionFrameHasZeroBlockKey) {
    // push a Frame for the main function and serialize it to channel
    this->serializeMainFunctionFrame();

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        uint32_t block_key = parsed["block_key"];
        ASSERT_EQ(block_key, 0)
            << "Func Frame should not have a zero block key";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << " Frame `block_key` value cannot be converted to uint32_t\n"
               << "Exception message: " << e.what()
               << this->fullErrorMessage("");
    }
}

TEST_F(FrameJSONSerialization, FirstFrameHasNoReturnAddress) {
    // push a Frame for the main function and serialize it to channel
    this->serializeMainFunctionFrame();

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        int frame_return_address = parsed["ra"];
        ASSERT_EQ(frame_return_address, -1)
            << "First Frame should no return address";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << " Frame `ra` value cannot be converted to int\n"
               << "Exception message: " << e.what()
               << this->fullErrorMessage("");
    }
}

TEST_F(FrameJSONSerialization, NoReturnAddressSerializesToNegativeOne) {
    Block block{};
    Frame frame{.block = &block, .ra_ptr = nullptr};

    int frame_index = 0;  // random
    this->serialization.serializeFrame(this->output->getChannel(), &frame,
                                       frame_index, this->wasm_module);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        int frame_return_address = parsed["ra"];
        ASSERT_EQ(frame_return_address, -1)
            << "First Frame should no return address";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << " Frame `ra` value cannot be converted to int\n"
               << "Exception message: " << e.what()
               << this->fullErrorMessage("");
    }
}

TEST_F(FrameJSONSerialization, NoStartAddressSerializesToZero) {
    Block block{.start_ptr = nullptr};
    Frame frame{.block = &block};

    int frame_index = 0;  // random
    this->serialization.serializeFrame(this->output->getChannel(), &frame,
                                       frame_index, this->wasm_module);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        uint32_t frame_start_address = parsed["start"];
        ASSERT_EQ(frame_start_address, 0)
            << "Frame without start address should return zero as address";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << " Frame `start` value cannot be converted to int\n"
               << "Exception message: " << e.what()
               << this->fullErrorMessage("");
    }
}

TEST_F(FrameJSONSerialization, NoReturnAddressGivesNoCallsite) {
    Block block{};
    Frame frame{.block = &block, .ra_ptr = nullptr};

    int frame_index = 0;  // random
    this->serialization.serializeFrame(this->output->getChannel(), &frame,
                                       frame_index, this->wasm_module);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        int frame_callsite = parsed["callsite"];
        ASSERT_EQ(frame_callsite, -1)
            << "Frame without return address should have -1 callsite";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << " Frame `callsite` value cannot be converted to int\n"
               << "Exception message: " << e.what()
               << this->fullErrorMessage("");
    }
}

TEST_F(FrameJSONSerialization, CallsiteIsTwoLessThanReturnAddress) {
    Block* block = this->moduleCompanion->getMainFunction();
    uint8_t* return_address = block->end_ptr;  // randomly chosen
    uint32_t expected_frame_return_address =
        toVirtualAddress(return_address, this->wasm_module);
    Frame frame{.block = block, .ra_ptr = return_address};

    int frame_index = 0;  // random
    this->serialization.serializeFrame(this->output->getChannel(), &frame,
                                       frame_index, this->wasm_module);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        int frame_callsite = parsed["callsite"];
        ASSERT_EQ(frame_callsite, expected_frame_return_address - 2)
            << "Frame callsite should be -2 of return address";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << " Frame `callsite` value cannot be converted to int\n"
               << "Exception message: " << e.what()
               << this->fullErrorMessage("");
    }
}

TEST_F(FrameJSONSerialization, ProxyGuardFrame) {
    const uint8_t proxy_block_type = 0xFF;
    Block block{.block_type = proxy_block_type};
    Frame frame{.block = &block, .ra_ptr = nullptr};

    int frame_index = 0;  // random
    this->serialization.serializeFrame(this->output->getChannel(), &frame,
                                       frame_index, this->wasm_module);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        uint8_t block_type = parsed["type"];
        uint32_t block_key = parsed["block_key"];
        ASSERT_EQ(block_type, proxy_block_type) << "Invalid frame type";
        ASSERT_EQ(block_key, 0) << "Function Block key should be zero";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << " Frame `type` or `block_key` value cannot be converted to "
                  "resp. uint8_t and uint32_t\n"
               << "Exception message: " << e.what()
               << this->fullErrorMessage("");
    }
}

TEST_F(FrameJSONSerialization, CallbackGuardFrame) {
    uint8_t event_block_type = 0xfe;
    Block block{.block_type = event_block_type};
    Frame frame{.block = &block, .ra_ptr = nullptr};

    int frame_index = 0;  // random
    this->serialization.serializeFrame(this->output->getChannel(), &frame,
                                       frame_index, this->wasm_module);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Frame serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        uint8_t block_type = parsed["type"];
        uint32_t block_key = parsed["block_key"];
        ASSERT_EQ(block_type, event_block_type) << "Invalid frame type";
        ASSERT_EQ(block_key, 0) << "Function Block key should be zero";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << " Frame `type` or `block_key` value cannot be converted to "
                  "resp. uint8_t and uint32_t\n"
               << "Exception message: " << e.what()
               << this->fullErrorMessage("");
    }
}
TEST_F(FrameJSONSerialization, CallstackFullySerialized) {
    // 3 random frames
    Frame frames[3] = {};
    for (int i = 0; i < 3; ++i) {
        Frame* frame = &frames[i];
        frame->block = this->moduleCompanion->getMainFunction();
        frame->ra_ptr = nullptr;
        frame->sp = i;
        frame->fp = i;
    }

    this->serialization.serializeCallstack(this->output->getChannel(), frames,
                                           0, 2, this->wasm_module);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Callstack serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        std::vector<nlohmann::json> callstack = parsed["callstack"];
        ASSERT_EQ(callstack.size(), 3);
        int expectedSP = 0;
        int expectedFP = 0;
        int expectedIndex = 0;
        for (auto jsonFrame : callstack) {
            EXPECT_EQ(expectedIndex++, jsonFrame["idx"]);
            EXPECT_EQ(expectedFP++, jsonFrame["fp"]);
            EXPECT_EQ(expectedSP++, jsonFrame["sp"]);
        }

    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(FrameJSONSerialization, CallstackOnlyBetweenStartAndEnd) {
    // 3 random frames
    Frame frames[3] = {};
    for (int i = 0; i < 3; ++i) {
        Frame* frame = &frames[i];
        frame->block = this->moduleCompanion->getMainFunction();
        frame->ra_ptr = nullptr;
        frame->sp = i;
        frame->fp = i;
    }

    int indexFirstFrame = 1;
    int indexLastFrame = 2;
    this->serialization.serializeCallstack(this->output->getChannel(), frames,
                                           indexFirstFrame, indexLastFrame,
                                           this->wasm_module);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Callstack serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    try {
        std::vector<nlohmann::json> callstack = parsed["callstack"];
        int expectedSize = indexLastFrame - indexFirstFrame + 1;
        ASSERT_EQ(callstack.size(), expectedSize);
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

using StackValueJSONSerialization = JSONSerialization;
TEST_F(StackValueJSONSerialization, HasIndex) {
    // random value
    StackValue value{};
    value.value.f32 = 0.3;
    value.value_type = F32;

    // random index
    uint32_t value_index = 44;

    this->serialization.serializeStackValue(this->output->getChannel(), value,
                                            value_index);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "StackValue serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsKey("idx"));

    try {
        uint32_t index = parsed["idx"];
        ASSERT_EQ(index, value_index) << "Stackvalue index do not match\n";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(StackValueJSONSerialization, HasValue) {
    // random value
    StackValue value{};
    value.value.f32 = 0.3;
    value.value_type = F32;

    // random index
    uint32_t value_index = 44;

    this->serialization.serializeStackValue(this->output->getChannel(), value,
                                            value_index);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "StackValue serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsKey("value"))
        << this->fullErrorMessage("Has no key `value`.\n");
}

TEST_F(StackValueJSONSerialization, UnsignedInteger32) {
    uint32_t u32 = 22;
    StackValue value = {.value_type = I32, .value.uint32 = u32};
    uint32_t valueIndex = 33;  // random

    this->serialization.serializeStackValue(this->output->getChannel(), value,
                                            valueIndex);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Value serialization JSON reply not received.");

    try {
        uint32_t parsed_value = parsed["value"];
        ASSERT_EQ(parsed_value, value.value.uint32);

        std::string parsed_type = parsed["type"];
        ASSERT_EQ(parsed_type, "i32");
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(StackValueJSONSerialization, NegativeSignedInteger32) {
    int32_t negativeValue = -22;
    StackValue value = {.value_type = I32, .value.int32 = negativeValue};
    uint32_t valueIndex = 33;  // random

    this->serialization.serializeStackValue(this->output->getChannel(), value,
                                            valueIndex);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Value serialization JSON reply not received.");

    try {
        int32_t parsed_value = parsed["value"];
        ASSERT_EQ(parsed_value, value.value.int32);

        std::string parsed_type = parsed["type"];
        ASSERT_EQ(parsed_type, "i32");
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(StackValueJSONSerialization, PositiveFloat32) {
    float val = 0.75;
    StackValue value = {.value_type = F32, .value.f32 = val};
    uint32_t valueIndex = 33;  // random

    this->serialization.serializeStackValue(this->output->getChannel(), value,
                                            valueIndex);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Value serialization JSON reply not received.");

    try {
        std::string parsed_value_hexa = parsed["value"];
        StackValue parsed_value{};
        ASSERT_TRUE(
            this->stackBuilder->F32FromHexa(parsed_value, parsed_value_hexa))
            << "Invalid hexa string representation of F32 value";
        ASSERT_EQ(val, value.value.f32);

        std::string parsed_type = parsed["type"];
        ASSERT_EQ(parsed_type, "F32");
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(StackValueJSONSerialization, NegativeFloat32) {
    float val = -0.75;
    StackValue value = {.value_type = F32, .value.f32 = val};
    uint32_t valueIndex = 33;  // random

    this->serialization.serializeStackValue(this->output->getChannel(), value,
                                            valueIndex);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Value serialization JSON reply not received.");

    try {
        std::string parsed_value_hexa = parsed["value"];
        StackValue parsed_value{};
        ASSERT_TRUE(
            this->stackBuilder->F32FromHexa(parsed_value, parsed_value_hexa))
            << "Invalid hexa string representation of F32 value";
        ASSERT_EQ(val, value.value.f32);

        std::string parsed_type = parsed["type"];
        ASSERT_EQ(parsed_type, "F32");
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(StackValueJSONSerialization, PositiveUnsignedInteger64) {
    uint64_t val = 1222;
    StackValue value = {.value_type = I64, .value.uint64 = val};
    uint32_t valueIndex = 33;  // random

    this->serialization.serializeStackValue(this->output->getChannel(), value,
                                            valueIndex);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Value serialization JSON reply not received.");

    try {
        uint64_t parsed_value = parsed["value"];
        ASSERT_EQ(parsed_value, value.value.uint64);

        std::string parsed_type = parsed["type"];
        ASSERT_EQ(parsed_type, "i64");
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(StackValueJSONSerialization, NegativeSignedInteger64) {
    int64_t val = -1222;
    StackValue value = {.value_type = I64, .value.int64 = val};
    uint32_t valueIndex = 33;  // random

    this->serialization.serializeStackValue(this->output->getChannel(), value,
                                            valueIndex);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Value serialization JSON reply not received.");

    try {
        int64_t parsed_value = parsed["value"];
        ASSERT_EQ(parsed_value, value.value.int64);

        std::string parsed_type = parsed["type"];
        ASSERT_EQ(parsed_type, "i64");
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(StackValueJSONSerialization, PostiveFloat64) {
    double val = 0.00000007;
    StackValue value = {.value_type = F64, .value.f64 = val};
    uint32_t valueIndex = 33;  // random

    this->serialization.serializeStackValue(this->output->getChannel(), value,
                                            valueIndex);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Value serialization JSON reply not received.");

    try {
        std::string parsed_value_hex = parsed["value"];
        StackValue parsed_value{};
        ASSERT_TRUE(
            this->stackBuilder->F64FromHexa(parsed_value, parsed_value_hex))
            << "Invalid hexastring for F64\n";
        ASSERT_EQ(parsed_value.value.f64, value.value.f64);

        std::string parsed_type = parsed["type"];
        ASSERT_EQ(parsed_type, "F64");
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(StackValueJSONSerialization, NegativeFloat64) {
    double val = -0.00000007;
    StackValue value = {.value_type = F64, .value.f64 = val};
    uint32_t valueIndex = 33;  // random

    this->serialization.serializeStackValue(this->output->getChannel(), value,
                                            valueIndex);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Value serialization JSON reply not received.");

    try {
        std::string parsed_value_hex = parsed["value"];
        StackValue parsed_value{};
        ASSERT_TRUE(
            this->stackBuilder->F64FromHexa(parsed_value, parsed_value_hex))
            << "Invalid hexastring for F64\n";
        ASSERT_EQ(parsed_value.value.f64, value.value.f64);

        std::string parsed_type = parsed["type"];
        ASSERT_EQ(parsed_type, "F64");
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(StackValueJSONSerialization, StackFullySerialized) {
    uint32_t stackSize = 5;
    StackValue* stack = this->stackBuilder->createStack(stackSize);
    for (int i = 0; i < stackSize; ++i) {
        StackValue* stackValue = &stack[i];
        *stackValue = {.value_type = F32, .value.f32 = 0.3};
    }
    this->serialization.serializeStack(this->output->getChannel(), stack, 0,
                                       stackSize - 1);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Stack serialization JSON reply not received.");

    try {
        std::vector<nlohmann::json> stack = parsed["stack"];
        ASSERT_EQ(stack.size(), stackSize);
        int expectedIndex = 0;
        for (auto jsonStackValue : stack) {
            EXPECT_EQ(expectedIndex, jsonStackValue["idx"])
                << "Index of StackValue does not match expected one";
            expectedIndex++;
        }

    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(StackValueJSONSerialization, StackSerializedWithinRange) {
    uint32_t stackSize = 8;
    StackValue* stack = this->stackBuilder->createStack(stackSize);
    for (int i = 0; i < stackSize; ++i) {
        StackValue* stackValue = &stack[i];
        *stackValue = {.value_type = F32, .value.f32 = 0.3};
    }
    int firstStackValueIndex = 5;
    int lastStackValueIndex = 7;
    this->serialization.serializeStack(this->output->getChannel(), stack,
                                       firstStackValueIndex,
                                       lastStackValueIndex);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Stack serialization JSON reply not received.");

    try {
        std::vector<nlohmann::json> stack = parsed["stack"];
        int expectedSize = lastStackValueIndex - firstStackValueIndex + 1;
        ASSERT_EQ(stack.size(), expectedSize);
        int expectedIndex = firstStackValueIndex;
        for (auto jsonStackValue : stack) {
            EXPECT_EQ(expectedIndex, jsonStackValue["idx"])
                << "Index of StackValue does not match expected one";
            expectedIndex++;
        }

    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

using GlobalValueJSONSerialization = JSONSerialization;
TEST_F(GlobalValueJSONSerialization, GlobalsFullySerialized) {
    uint32_t globalsSize = 5;
    StackValue* globals = this->stackBuilder->createStack(globalsSize);
    for (int i = 0; i < globalsSize; ++i) {
        StackValue* global = &globals[i];
        *global = {.value_type = F32, .value.f32 = 0.3};
    }
    this->serialization.serializeGlobals(this->output->getChannel(), globals, 0,
                                         globalsSize);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Globals serialization JSON reply not received.");

    try {
        std::vector<nlohmann::json> parsed_globals = parsed["globals"];
        ASSERT_EQ(parsed_globals.size(), globalsSize);
        int expectedIndex = 0;
        for (auto jsonGlobal : parsed_globals) {
            EXPECT_EQ(expectedIndex, jsonGlobal["idx"])
                << "Index of Global does not match expected one";
            expectedIndex++;
        }
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(GlobalValueJSONSerialization, GlobalsOnlyBetweenStartAndEnd) {
    uint32_t globalsSize = 8;
    StackValue* globals = this->stackBuilder->createStack(globalsSize);
    for (int i = 0; i < globalsSize; ++i) {
        StackValue* global = &globals[i];
        *global = {.value_type = F32, .value.f32 = 0.3};
    }

    int firstGlobalIndex = 5;
    int lastGlobalIndex = 7;
    this->serialization.serializeGlobals(this->output->getChannel(), globals,
                                         firstGlobalIndex, lastGlobalIndex);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Globals serialization JSON reply not received.");

    try {
        std::vector<nlohmann::json> parsed_globals = parsed["globals"];
        int expectedSize = lastGlobalIndex - firstGlobalIndex;
        ASSERT_EQ(parsed_globals.size(), expectedSize);
        int expectedIndex = firstGlobalIndex;
        for (auto jsonGlobal : parsed_globals) {
            EXPECT_EQ(expectedIndex, jsonGlobal["idx"])
                << "Index of Global does not match expected one";
            expectedIndex++;
        }
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

using EventJSONSerialization = JSONSerialization;
TEST_F(EventJSONSerialization, FullEventSerialization) {
    // random value
    std::string topic{"some topic"};
    std::string payload{"some payload"};
    const Event e{topic, payload};

    this->serialization.serializeEvent(this->output->getChannel(), e);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Event serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsKey("topic"))
        << this->fullErrorMessage("Has no key `topic`.\n");
    ASSERT_TRUE(companion.containsKey("payload"))
        << this->fullErrorMessage("Has no key `payload`.\n");
    ASSERT_TRUE(companion.containsOnlyKeys({"topic", "payload"}))
        << this->fullErrorMessage("Has no more keys than expected\n");
    try {
        std::string parsed_topic = parsed["topic"];
        std::string parsed_payload = parsed["payload"];
        ASSERT_EQ(parsed_topic, topic);
        ASSERT_EQ(parsed_payload, payload);
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(EventJSONSerialization, EventsBetweenStartAndEnd) {
    Event ev1{"topic 1", "payload 1"};
    Event ev2{"topic 2", "payload 2"};
    Event ev3{"topic 3", "payload 3"};
    CallbackHandler::push_event(&ev1);
    CallbackHandler::push_event(&ev2);
    CallbackHandler::push_event(&ev3);

    std::deque<Event>::const_iterator start = CallbackHandler::event_begin();
    std::deque<Event>::const_iterator end = CallbackHandler::event_end();
    this->serialization.serializeEvents(this->output->getChannel(), start, end);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Event serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"events"}));

    try {
        std::vector<nlohmann::json> parsed_events = parsed["events"];
        int expectedSize = std::distance(start, end);
        ASSERT_EQ(parsed_events.size(), expectedSize);
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

using TableJSONSerialization = JSONSerialization;
TEST_F(TableJSONSerialization, HasIntMax) {
    // Random table
    const int maxSize = 7;
    const int initSize = 2;
    uint32_t elements[5] = {0, 1, 2, 3, 4};
    Table table{};
    table.elem_type = 0;
    table.initial = initSize;
    table.maximum = maxSize;
    table.size = 5;
    table.entries = elements;

    this->serialization.serializeTable(this->output->getChannel(), table);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Table serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"table"}));

    try {
        nlohmann::json parsed_table = parsed["table"];
        int parsed_max = parsed_table["max"];
        ASSERT_EQ(parsed_max, maxSize);
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(TableJSONSerialization, HasIntInitialSize) {
    // Random table
    const int maxSize = 7;
    const int initSize = 2;
    uint32_t elements[5] = {0, 1, 2, 3, 4};
    Table table{};
    table.elem_type = 0;
    table.initial = initSize;
    table.maximum = maxSize;
    table.size = 5;
    table.entries = elements;

    this->serialization.serializeTable(this->output->getChannel(), table);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Table serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"table"}));

    try {
        nlohmann::json parsed_table = parsed["table"];
        int parsed_init = parsed_table["init"];
        ASSERT_EQ(parsed_init, initSize);
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(TableJSONSerialization, HasTableElements) {
    // Random table
    const int maxSize = 7;
    const int initSize = 2;
    uint32_t elements[5] = {0, 1, 2, 3, 4};
    Table table{};
    table.elem_type = 0;
    table.initial = initSize;
    table.maximum = maxSize;
    table.size = 5;
    table.entries = elements;

    this->serialization.serializeTable(this->output->getChannel(), table);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Table serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"table"}));

    try {
        nlohmann::json parsed_table = parsed["table"];
        std::vector<uint32_t> parsed_elements = parsed_table["elements"];
        uint32_t expectedElement = 0;
        for (uint32_t val : parsed_elements) {
            EXPECT_EQ(expectedElement++, val) << "Table element do not match";
        }
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(TableJSONSerialization, HasAllKeys) {
    // Random table
    const int maxSize = 7;
    const int initSize = 2;
    uint32_t elements[5] = {0, 1, 2, 3, 4};
    Table table{};
    table.elem_type = 0;
    table.initial = initSize;
    table.maximum = maxSize;
    table.size = 5;
    table.entries = elements;

    this->serialization.serializeTable(this->output->getChannel(), table);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Table serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"table"}));
    try {
        JSONCompanion companion{parsed["table"]};
        ASSERT_TRUE(companion.containsOnlyKeys({"max", "init", "elements"}));
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

using BranchingTableJSONSerialization = JSONSerialization;
TEST_F(BranchingTableJSONSerialization, HasAllKeys) {
    uint32_t labels[BR_TABLE_SIZE]{};
    for (int i = 0; i < BR_TABLE_SIZE; ++i) {
        labels[i] = i;
    }

    this->serialization.serializeBranchingTable(this->output->getChannel(),
                                                labels);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Table serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"br_table"}));

    try {
        nlohmann::json br_table = parsed["br_table"];
        JSONCompanion companion{br_table};
        ASSERT_TRUE(companion.containsOnlyKeys({"size", "labels"}));
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(BranchingTableJSONSerialization, HasSizeAsHexString) {
    uint32_t labels[BR_TABLE_SIZE]{};
    for (int i = 0; i < BR_TABLE_SIZE; ++i) {
        labels[i] = i;
    }

    this->serialization.serializeBranchingTable(this->output->getChannel(),
                                                labels);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Table serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"br_table"}));

    try {
        nlohmann::json br_table = parsed["br_table"];
        std::string parsed_size = br_table["size"];
        ASSERT_EQ(parsed_size, this->numberToHexString(BR_TABLE_SIZE));
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(BranchingTableJSONSerialization, HasLabelsAsUnsingedIntegerArray) {
    uint32_t labels[BR_TABLE_SIZE]{};
    for (int i = 0; i < BR_TABLE_SIZE; ++i) {
        labels[i] = i;
    }

    this->serialization.serializeBranchingTable(this->output->getChannel(),
                                                labels);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Table serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"br_table"}));

    try {
        nlohmann::json br_table = parsed["br_table"];
        std::vector<uint32_t> parsed_labels = br_table["labels"];
        ASSERT_EQ(parsed_labels.size(), BR_TABLE_SIZE);

        uint32_t expectedLabelValue = 0;
        for (uint32_t val : parsed_labels) {
            EXPECT_EQ(expectedLabelValue++, val)
                << "Branching Table element do not match";
        }
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

using MemoryJSONSerialization = JSONSerialization;
TEST_F(MemoryJSONSerialization, HasAllKeys) {
    Memory mem{};
    mem.maximum = 3;   // random
    mem.pages = 1;     // random
    mem.initial = 25;  // random

    uint8_t mem_bytes[PAGE_SIZE]{};
    uint8_t val = 0;
    for (int i = 0; i < PAGE_SIZE; ++i) {
        mem_bytes[i] = val;
        val = (++val) % 255;
    }
    mem.bytes = mem_bytes;

    this->serialization.serializeMemory(this->output->getChannel(), mem);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Memory serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"memory"}));

    try {
        nlohmann::json parsed_memory = parsed["memory"];
        JSONCompanion companion{parsed_memory};
        ASSERT_TRUE(
            companion.containsOnlyKeys({"pages", "max", "init", "bytes"}));
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(MemoryJSONSerialization, MaxIsInteger) {
    Memory mem{};
    mem.maximum = 3;   // random
    mem.pages = 1;     // random
    mem.initial = 25;  // random

    uint8_t mem_bytes[PAGE_SIZE]{};
    uint8_t val = 0;
    for (int i = 0; i < PAGE_SIZE; ++i) {
        mem_bytes[i] = val;
        val = (++val) % 255;
    }
    mem.bytes = mem_bytes;

    this->serialization.serializeMemory(this->output->getChannel(), mem);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Memory serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"memory"}));

    try {
        nlohmann::json parsed_memory = parsed["memory"];
        int parsed_max = parsed_memory["max"];
        ASSERT_EQ(parsed_max, mem.maximum);
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}
TEST_F(MemoryJSONSerialization, InitIsInteger) {
    Memory mem{};
    mem.maximum = 3;   // random
    mem.pages = 1;     // random
    mem.initial = 25;  // random

    uint8_t mem_bytes[PAGE_SIZE]{};
    uint8_t val = 0;
    for (int i = 0; i < PAGE_SIZE; ++i) {
        mem_bytes[i] = val;
        val = (++val) % 255;
    }
    mem.bytes = mem_bytes;

    this->serialization.serializeMemory(this->output->getChannel(), mem);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Memory serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"memory"}));

    try {
        nlohmann::json parsed_memory = parsed["memory"];
        int parsed_initial_size = parsed_memory["init"];
        ASSERT_EQ(parsed_initial_size, mem.initial);
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(MemoryJSONSerialization, BytesIsUnsinged8BitArray) {
    Memory mem{};
    mem.maximum = 3;   // random
    mem.pages = 1;     // random
    mem.initial = 25;  // random

    uint8_t mem_bytes[PAGE_SIZE]{};
    uint8_t val = 0;
    for (int i = 0; i < PAGE_SIZE * mem.pages; ++i) {
        mem_bytes[i] = val;
        val = (++val) % 255;
    }
    mem.bytes = mem_bytes;

    this->serialization.serializeMemory(this->output->getChannel(), mem);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "Memory serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"memory"}));

    try {
        nlohmann::json parsed_memory = parsed["memory"];
        std::vector<uint8_t> parsed_bytes = parsed_memory["bytes"];

        ASSERT_EQ(parsed_bytes.size(), PAGE_SIZE * mem.pages);

        uint32_t i = 0;
        for (uint8_t byte_val : parsed_bytes) {
            ASSERT_EQ(byte_val, mem_bytes[i++]);
        }
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

using CallbackMappingJSONSerialization = JSONSerialization;
TEST_F(CallbackMappingJSONSerialization, HasAllKeys) {
    std::string triggerTopic = "topic that triggers callback";
    uint32_t numberOfCallbacks = 4;
    std::vector<Callback> callbacks{};
    for (uint32_t i = 0; i < numberOfCallbacks; ++i) {
        callbacks.push_back(Callback{this->wasm_module, triggerTopic, i});
    }
    this->serialization.serializeCallbackMapping(this->output->getChannel(),
                                                 triggerTopic, &callbacks);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "CallbackMapping serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({triggerTopic}))
        << this->fullErrorMessage("JSON has not the expected key. ");
}

TEST_F(CallbackMappingJSONSerialization, HasTableIndexesAsUnsinged32Integer) {
    std::string triggerTopic = "topic that triggers callback";
    uint32_t numberOfCallbacks = 4;
    std::vector<Callback> callbacks{};
    for (uint32_t i = 0; i < numberOfCallbacks; ++i) {
        callbacks.push_back(Callback{this->wasm_module, triggerTopic, i});
    }
    this->serialization.serializeCallbackMapping(this->output->getChannel(),
                                                 triggerTopic, &callbacks);

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "CallbackMapping serialization JSON reply not received.");

    try {
        std::vector<uint32_t> table_indexes = parsed[triggerTopic];
        ASSERT_EQ(table_indexes.size(), numberOfCallbacks)
            << "One table index per callback is expected";

        uint32_t i = 0;
        for (uint32_t table_index : table_indexes) {
            ASSERT_EQ(table_index, i++);
        }
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(CallbackMappingJSONSerialization, CallbacksForOneTopicGivesArraOfSize1) {
    std::string triggerTopic = "topic that triggers callback";
    uint32_t numberOfCallbacks = 4;
    for (uint32_t i = 0; i < numberOfCallbacks; ++i) {
        const Callback cb{this->wasm_module, triggerTopic, i};
        CallbackHandler::add_callback(cb);
    }
    this->serialization.serializeCallbackMappings(
        this->output->getChannel(), CallbackHandler::callbacks_begin(),
        CallbackHandler::callbacks_end());

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "CallbacksMappings serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"callbacks"}))
        << this->fullErrorMessage("JSON has not expected format");

    try {
        std::vector<nlohmann::json> parsed_callbacks = parsed["callbacks"];
        ASSERT_EQ(parsed_callbacks.size(), 1);

        std::vector<nlohmann::json> table_indexes =
            parsed_callbacks[0][triggerTopic];
        ASSERT_EQ(table_indexes.size(), numberOfCallbacks);

    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(CallbackMappingJSONSerialization, CallbacksForTwoTopicGivesArraOfSize2) {
    std::string triggerTopicA = "topic A";
    uint32_t numberOfCallbacksForA = 4;
    for (uint32_t i = 0; i < numberOfCallbacksForA; ++i) {
        const Callback cb{this->wasm_module, triggerTopicA, i};
        CallbackHandler::add_callback(cb);
    }

    std::string triggerTopicB = "topic B";
    uint32_t numberOfCallbacksForB = 2;
    for (uint32_t i = 0; i < numberOfCallbacksForB; ++i) {
        const Callback cb{this->wasm_module, triggerTopicB, i};
        CallbackHandler::add_callback(cb);
    }

    this->serialization.serializeCallbackMappings(
        this->output->getChannel(), CallbackHandler::callbacks_begin(),
        CallbackHandler::callbacks_end());

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "CallbacksMappings serialization JSON reply not received.");

    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"callbacks"}));

    try {
        std::vector<nlohmann::json> parsed_callbacks = parsed["callbacks"];
        ASSERT_EQ(parsed_callbacks.size(), 2)
            << this->fullErrorMessage("Callbacks for topic A and B expected");

        nlohmann::json callbacks_A = parsed_callbacks[0].contains(triggerTopicA)
                                         ? parsed_callbacks[0]
                                         : parsed_callbacks[1];
        JSONCompanion companionA{callbacks_A};
        ASSERT_TRUE(companionA.containsOnlyKeys({triggerTopicA}))
            << "JSON object is:\n"
            << callbacks_A << "\n";

        std::vector<uint32_t> indexes_A = callbacks_A[triggerTopicA];
        ASSERT_EQ(indexes_A.size(), numberOfCallbacksForA);
        uint32_t i = 0;
        for (uint32_t table_index : indexes_A) {
            ASSERT_EQ(table_index, i++);
        }

        nlohmann::json callbacks_B = parsed_callbacks[0].contains(triggerTopicB)
                                         ? parsed_callbacks[0]
                                         : parsed_callbacks[1];
        JSONCompanion companionB{callbacks_B};
        ASSERT_TRUE(companionB.containsOnlyKeys({triggerTopicB}));

        std::vector<uint32_t> indexes_B = callbacks_B[triggerTopicB];
        ASSERT_EQ(indexes_B.size(), numberOfCallbacksForB);
        uint32_t j = 0;
        for (uint32_t table_index : indexes_B) {
            ASSERT_EQ(table_index, j++);
        }
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(CallbackMappingJSONSerialization, MappingVers2HasAllKeys) {
    std::string triggerTopic = "topic that triggers callback";
    uint32_t numberOfCallbacks = 4;
    std::vector<Callback> callbacks{};
    for (uint32_t i = 0; i < numberOfCallbacks; ++i) {
        CallbackHandler::add_callback(
            Callback{this->wasm_module, triggerTopic, i});
    }
    this->serialization.serializeCallbackMappingsVers2(
        this->output->getChannel(), CallbackHandler::callbacks_begin(),
        CallbackHandler::callbacks_end());

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "CallbackMapping serialization JSON reply not received.");
    JSONCompanion companion{parsed};
    ASSERT_TRUE(companion.containsOnlyKeys({"callbacks"}))
        << "JSON object is:\n"
        << parsed << "\n";

    try {
        nlohmann::json callbacks = parsed["callbacks"][0];
        JSONCompanion companion{callbacks};
        ASSERT_TRUE(companion.containsOnlyKeys({"callbackid", "tableIndexes"}))
            << "JSON object is:\n"
            << parsed << "\n";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(CallbackMappingJSONSerialization, InVers2CallbackIDIsTopic) {
    std::string triggerTopic = "topic that triggers callback";
    uint32_t numberOfCallbacks = 4;
    std::vector<Callback> callbacks{};
    for (uint32_t i = 0; i < numberOfCallbacks; ++i) {
        CallbackHandler::add_callback(
            Callback{this->wasm_module, triggerTopic, i});
    }
    this->serialization.serializeCallbackMappingsVers2(
        this->output->getChannel(), CallbackHandler::callbacks_begin(),
        CallbackHandler::callbacks_end());

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "CallbackMapping serialization JSON reply not received.");

    try {
        nlohmann::json callbacks = parsed["callbacks"][0];
        ASSERT_EQ(callbacks["callbackid"], triggerTopic) << "JSON object is:\n"
                                                         << callbacks << "\n";
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

TEST_F(CallbackMappingJSONSerialization, InVers2HasTableIndexes) {
    std::string triggerTopic = "topic that triggers callback";
    uint32_t numberOfCallbacks = 4;
    std::vector<Callback> callbacks{};
    for (uint32_t i = 0; i < numberOfCallbacks; ++i) {
        CallbackHandler::add_callback(
            Callback{this->wasm_module, triggerTopic, i});
    }
    this->serialization.serializeCallbackMappingsVers2(
        this->output->getChannel(), CallbackHandler::callbacks_begin(),
        CallbackHandler::callbacks_end());

    nlohmann::basic_json<> parsed{};
    ASSERT_TRUE(this->output->getJSONReply(&parsed)) << this->fullErrorMessage(
        "CallbackMapping serialization JSON reply not received.");

    try {
        nlohmann::json callbacks = parsed["callbacks"][0];
        std::vector<uint32_t> table_indexes = callbacks["tableIndexes"];
        ASSERT_EQ(table_indexes.size(), numberOfCallbacks)
            << "Received indexes: " << parsed["tableIndexes"] << "\n";
        uint32_t j = 0;
        for (uint32_t table_index : table_indexes) {
            ASSERT_EQ(table_index, j++);
        }
    } catch (const nlohmann::detail::type_error& e) {
        FAIL() << this->fullErrorMessage("Exception occurred. ")
               << "Exception message: " << e.what();
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
