/* Versioned frame-action fixtures: independent bytecode blocks, ordinary ActionCall and native setup. */
#define main frame_timeline_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_movieclip_timeline.cpp"

#undef main
#include "movieclip_tick_fixture.hpp"
#include "flashvm/execution.h"
#include "flashvm/jit.h"
#include "../src/standard/movieclip_state.hpp"
#include "../src/standard/movieclip_storage.hpp"


static FVM_Value frame_hex(FVM_Context *c, const Bytes &bytes)
{
    static const char hex[] = "0123456789abcdef";
    fvm_detail::vector<char> text;
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        text.push_back(hex[bytes[i] >> 4]);
        text.push_back(hex[bytes[i] & 15]);
    }
    FVM_Value value;
    if (!(fvm_value_string_bytes(&value, text.begin(), text.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *holder = 0;
    if (!(fvm_object_create(c, &holder) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, holder, "code", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return holder->properties["code"];
}


static FVM_Object *frame_segments(FVM_Context *c, FVM_Object *clip, unsigned frame = 1)
{
    FVM_Object *frames = clip->resource_result.object->properties["frames"].object;
    const unsigned char empty[] = {0};
    if (!(fvm_movieclip_define_frame(c, clip, frame, 0, empty, sizeof(empty)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *segments = 0;
    if (!(fvm_object_create(c, &segments) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    frames->indexed[frame].object->properties["segments"] = tick_object(segments);
    return segments;
}


static Bytes frame_call(unsigned frame = 1)
{
    Bytes code;
    code.push_back(0x96);
    u16(code, 5);
    code.push_back(7);
    u32(code, frame);
    code.push_back(0x9e);
    u16(code, 0);
    code.push_back(0);
    return code;
}


/* Distinguish direct native frame entry, public execution service and interpreted/native ActionCall. */
static FVM_Result run_frame(FVM_Context *c, FVM_Object *clip, unsigned route)
{
    FVM_Value argument;
    fvm_value_number(&argument, 1);
    if (route == 0)
    {
        return fvm_movieclip_call_frame(c, clip, argument);
    }
    if (route == 1)
    {
        return fvm_exec_call_frame(c, clip, &argument);
    }
    if (route == 4)
    {
        return fvm_movieclip_advance(c, clip, 1);
    }
    if (!(fvm_execution_set_mode(c, route == 3 ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes call = frame_call();
    return fvm_avm1_execute_receiver(c, call.begin(), call.size(), clip);
}
