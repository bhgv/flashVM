#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/serialization.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
static FVM_Value call(FVM_Context *c, FVM_Object *o, const char *method, double depth)
{
    FVM_Value value, out; fvm_value_number(&value, depth);
    assert(fvm_object_call(c, o, method, 1, &value, &out) == FVM_OK); return out;
}
static FVM_Object *child(FVM_Context *c, FVM_Object *p, const char *name, double depth)
{
    FVM_Value a[2], out; fvm_value_string(a, name); fvm_value_number(a + 1, depth);
    assert(fvm_object_call(c, p, "createEmptyMovieClip", 2, a, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_OBJECT); return out.object;
}
static double depth(FVM_Context *c, FVM_Object *o)
{
    FVM_Value out; assert(fvm_object_call(c, o, "getDepth", 0, 0, &out) == FVM_OK); return out.number;
}
static FVM_Object *legacy(FVM_Context *c)
{
    char path[1024]; std::strcpy(path, __FILE__); char *slash = std::strrchr(path, '/');
    if (!slash) slash = std::strrchr(path, '\\');
    assert(slash);
    std::strcpy(slash + 1, "fixtures/movieclip-v026.fvg");
    std::FILE *file = std::fopen(path, "rb"); assert(file);
    assert(std::fseek(file, 0, SEEK_END) == 0); long n = std::ftell(file); assert(n > 0);
    assert(std::fseek(file, 0, SEEK_SET) == 0); unsigned char *bytes = (unsigned char *)std::malloc((size_t)n); assert(bytes);
    assert(std::fread(bytes, 1, (size_t)n, file) == (size_t)n); assert(std::fclose(file) == 0);
    FVM_Value value; assert(fvm_value_deserialize(c, bytes, (size_t)n, &value) == FVM_OK); std::free(bytes); return value.object;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0, *other = 0; FVM_Object *p = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK); assert(fvm_movieclip_create(c, "root", 1, &p) == FVM_OK);
    assert(fvm_gc_pin(c, p) == FVM_OK);
    FVM_Object *below = child(c, p, "below", -16384), *authored = child(c, p, "authored", -16383), *dynamic = child(c, p, "dynamic", 0);
    assert(depth(c, below) == -16384 && depth(c, authored) == -16383 && depth(c, dynamic) == 0);
    assert(call(c, p, "getInstanceAtDepth", -16384).object == below);
    assert(call(c, p, "getInstanceAtDepth", -16383).object == authored);
    assert(call(c, p, "getInstanceAtDepth", 0).object == dynamic);
    FVM_Value out; assert(fvm_object_call(c, p, "getNextHighestDepth", 0, 0, &out) == FVM_OK && out.number == 1);
    assert(call(c, below, "swapDepths", 0).type == FVM_VALUE_UNDEFINED);
    assert(depth(c, below) == 0 && depth(c, dynamic) == -16384);
    FVM_Value object; fvm_value_undefined(&object); object.type = FVM_VALUE_OBJECT; object.object = authored;
    assert(fvm_object_call(c, dynamic, "swapDepths", 1, &object, &out) == FVM_OK);
    assert(depth(c, dynamic) == -16383 && depth(c, authored) == -16384);
    assert(fvm_object_call(c, authored, "removeMovieClip", 0, 0, &out) == FVM_OK);
    assert(call(c, p, "getInstanceAtDepth", -16384).type == FVM_VALUE_UNDEFINED);
    FVM_Object *top = child(c, p, "top", 1048575);
    assert(call(c, p, "getInstanceAtDepth", 1048575).object == top);
    assert(fvm_object_call(c, p, "getNextHighestDepth", 0, 0, &out) == FVM_OK && out.number == 1048576);
    FVM_Value invalid[2]; fvm_value_string(invalid, "invalid"); fvm_value_number(invalid + 1, -16385);
    assert(fvm_object_call(c, p, "createEmptyMovieClip", 2, invalid, &out) == FVM_INVALID_ARGUMENT);
    fvm_value_number(invalid + 1, 1048576); assert(fvm_object_call(c, p, "createEmptyMovieClip", 2, invalid, &out) == FVM_INVALID_ARGUMENT);
    fvm_value_number(invalid + 1, -0.5); assert(fvm_object_call(c, p, "createEmptyMovieClip", 2, invalid, &out) == FVM_INVALID_ARGUMENT);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK);
    object.object = p; assert(fvm_channel_send_value(c, channel, &object, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); p = out.object;
    assert(fvm_gc_pin(other, p) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK);
    assert(depth(other, call(other, p, "getInstanceAtDepth", -16383).object) == -16383);
    FVM_Object *old = legacy(other); assert(fvm_gc_pin(other, old) == FVM_OK);
    FVM_Object *old_child = call(other, old, "getInstanceAtDepth", 2).object; assert(old_child);
    assert(call(other, old, "getInstanceAtDepth", -16384).type == FVM_VALUE_UNDEFINED);
    FVM_Object *new_child = child(other, old, "beforeUpgrade", 8);
    FVM_Object *negative = child(other, old, "negative", -16384);
    assert(call(other, old, "getInstanceAtDepth", 2).object == old_child);
    assert(call(other, old, "getInstanceAtDepth", 8).object == new_child);
    assert(call(other, old, "getInstanceAtDepth", -16384).object == negative);
    assert(depth(other, old_child) == 2 && depth(other, new_child) == 8);
    assert(fvm_gc_collect(other, 0) == FVM_OK);
    assert(fvm_object_call(other, old, "getNextHighestDepth", 0, 0, &out) == FVM_OK && out.number == 9);
    fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
