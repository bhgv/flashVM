/* addProperty accepts plain descriptor Objects; absent executable callbacks are harmless. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "flashvm/transfer.h"
#include <cassert>
static unsigned writes;
static FVM_Result get(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ fvm_value_number(out, 42); return FVM_OK; }
static FVM_Result set(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *args, FVM_Value *out, void *)
{ assert(n == 1 && args[0].type == FVM_VALUE_NUMBER && args[0].number == 81); ++writes; fvm_value_undefined(out); return FVM_OK; }
static FVM_Value object(FVM_Object *o)
{ FVM_Value v; fvm_value_undefined(&v); v.type = FVM_VALUE_OBJECT; v.object = o; return v; }
static void number(FVM_Context *c, FVM_Object *o, const char *name)
{ FVM_Value v; assert(fvm_object_get(c, o, name, &v) == FVM_OK && v.type == FVM_VALUE_NUMBER && v.number == 81); }
static void undefined(FVM_Context *c, FVM_Object *o, const char *name)
{ FVM_Value v; assert(fvm_object_get(c, o, name, &v) == FVM_OK && v.type == FVM_VALUE_UNDEFINED); }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0, *other = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    FVM_Object *owner = 0, *plain_get = 0, *plain_set = 0, *get_fn = 0, *set_fn = 0;
    assert(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK); assert(fvm_gc_pin(c, owner) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &plain_get) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &plain_set) == FVM_OK);
    assert(fvm_function_create_native(c, get, 0, 0, &get_fn) == FVM_OK); assert(fvm_gc_pin(c, get_fn) == FVM_OK);
    assert(fvm_function_create_native(c, set, 0, 0, &set_fn) == FVM_OK); assert(fvm_gc_pin(c, set_fn) == FVM_OK);
    FVM_Value args[3], out, value; fvm_value_string(args, "plain"); args[1] = object(plain_get); args[2] = object(plain_set);
    assert(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
    undefined(c, owner, "plain"); fvm_value_number(&value, 81);
    assert(fvm_object_set(c, owner, "plain", &value) == FVM_OK); number(c, owner, "plain");
    FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK);
    out = object(owner); assert(fvm_channel_send_value(c, channel, &out, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); assert(fvm_gc_pin(other, out.object) == FVM_OK);
    assert(fvm_gc_collect(other, 0) == FVM_OK); number(other, out.object, "plain");
    assert(fvm_object_set(other, out.object, "plain", &value) == FVM_OK); number(other, out.object, "plain");
    fvm_value_string(args, "read"); args[1] = object(get_fn);
    assert(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_set(c, owner, "read", &value) == FVM_OK);
    assert(fvm_object_get(c, owner, "read", &out) == FVM_OK && out.number == 42);
    fvm_value_string(args, "write"); args[1] = object(plain_get); args[2] = object(set_fn);
    assert(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_set(c, owner, "write", &value) == FVM_OK && writes == 1); number(c, owner, "write");
    fvm_value_string(args, "readonly"); fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL;
    assert(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
    unsigned flags = 0; assert(fvm_object_property_flags(c, owner, "readonly", &flags) == FVM_OK && (flags & 4));
    assert(fvm_object_set(c, owner, "readonly", &value) == FVM_OK); undefined(c, owner, "readonly");
    fvm_value_string(args, "invalid"); fvm_value_number(args + 1, 1);
    assert(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && !out.boolean);
    assert(fvm_gc_collect(c, 0) == FVM_OK); number(c, owner, "plain"); undefined(c, owner, "readonly");
    fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
