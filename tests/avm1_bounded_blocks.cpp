/* Exact SWF record boundaries allow implicit End without accepting truncated records. */
#define main timeline_fixture_main
#include "swf_movieclip_timeline.cpp"
#undef main
#include "flashvm/avm1.h"
#include "../src/avm1/block.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0, *other = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    Bytes code = action(73); code.pop_back(); Bytes original = code;
    FVM_AVM1Info info; info.struct_size = sizeof(info);
    assert(fvm_avm1_inspect(code.begin(), code.size(), &info) == FVM_INVALID_ARGUMENT);
    assert(fvm_avm1_inspect_block(code.begin(), code.size(), &info) == FVM_OK);
    unsigned long instructions = info.instruction_count;
    Bytes ended = code; ended.push_back(0);
    assert(fvm_avm1_inspect(ended.begin(), ended.size(), &info) == FVM_OK);
    assert(info.instruction_count == instructions + 1);
    assert(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_INVALID_ARGUMENT);
    assert(fvm_avm1_execute(c, 0, 0) == FVM_INVALID_ARGUMENT);
    Bytes tags, empty; tag(tags, 12, empty); tag(tags, 12, code);
    tag(tags, 1, empty); tag(tags, 0, empty); Bytes swf = movie(tags, 1);
    assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK);
    assert(fvm_avm1_execute(c, 0, 0) == FVM_OK);
    FVM_Object *clip = 0;
    assert(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &clip) == FVM_OK);
    assert(fvm_gc_pin(c, clip) == FVM_OK);
    assert(fvm_avm1_execute_receiver(c, code.begin(), code.size(), clip) == FVM_OK);
    number(c, clip, "seen", 73);
    assert(!std::memcmp(code.begin(), original.begin(), code.size()));
    assert(fvm_movieclip_advance(c, clip, 1) == FVM_OK);
    FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK);
    FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = clip;
    assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &value, 0) == FVM_OK);
    assert(fvm_gc_pin(other, value.object) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK);
    assert(fvm_avm1_execute_receiver(other, code.begin(), code.size(), value.object) == FVM_INVALID_ARGUMENT);
    assert(fvm_movieclip_advance(other, value.object, 1) == FVM_OK); number(other, value.object, "seen", 73);
    const unsigned char jump[] = {0x99, 2, 0, 0, 0};
    assert(fvm_avm1_execute(c, jump, sizeof(jump)) == FVM_OK);
    const unsigned char inside[] = {0x99, 2, 0, 0xff, 0xff};
    const unsigned char outside[] = {0x99, 2, 0, 1, 0};
    const unsigned char truncated[] = {0x96, 2, 0, 0};
    const unsigned char trailing[] = {0, 0x17};
    assert(fvm_avm1_execute(c, inside, sizeof(inside)) == FVM_INVALID_ARGUMENT);
    assert(fvm_avm1_execute(c, outside, sizeof(outside)) == FVM_INVALID_ARGUMENT);
    assert(fvm_avm1_execute(c, truncated, sizeof(truncated)) == FVM_INVALID_ARGUMENT);
    assert(fvm_avm1_execute(c, trailing, sizeof(trailing)) == FVM_INVALID_ARGUMENT);
    assert(fvm_avm1_inspect_block(truncated, sizeof(truncated), &info) == FVM_INVALID_ARGUMENT);
    fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
