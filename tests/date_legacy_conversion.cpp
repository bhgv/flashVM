#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static unsigned calls;
static FVM_Result operand(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++calls; fvm_value_number(out, 2); return FVM_OK; }
static FVM_Result failure(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{ ++calls; return FVM_ERROR; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value klass, args[8], numeric[7], out, value; FVM_Object *arg = 0, *fn = 0, *date = 0, *reference = 0;
        assert(fvm_get_global(c, "Date", &klass) == FVM_OK); assert(fvm_construct(c, "Object", 0, 0, &arg) == FVM_OK); assert(fvm_function_create_native(c, operand, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, arg, "valueOf", &value) == FVM_OK);
        for (unsigned i = 0; i < 8; ++i) args[i] = object(arg); for (unsigned i = 0; i < 7; ++i) fvm_value_number(numeric+i, 2);
        calls = 0; assert(fvm_construct(c, "Date", 8, args, &date) == FVM_OK && calls == 7); assert(fvm_construct(c, "Date", 7, numeric, &reference) == FVM_OK); assert(fvm_object_call(c, date, "getTime", 0, 0, &out) == FVM_OK); double timestamp = out.number; assert(fvm_object_call(c, reference, "getTime", 0, 0, &out) == FVM_OK && out.number == timestamp);
        calls = 0; assert(fvm_object_call(c, klass.object, "UTC", 8, args, &out) == FVM_OK && calls == 7); timestamp = out.number; assert(fvm_object_call(c, klass.object, "UTC", 7, numeric, &out) == FVM_OK && out.number == timestamp);
        assert(fvm_object_call(c, klass.object, "UTC", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED); fvm_value_number(args, 1970); assert(fvm_object_call(c, klass.object, "UTC", 1, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        assert(fvm_function_create_native(c, failure, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, arg, "valueOf", &value) == FVM_OK);
        fvm_value_number(args+1, 0); fvm_value_undefined(args+2); calls = 0; assert(fvm_object_call(c, klass.object, "UTC", 8, args, &out) == FVM_OK && out.number == 0 && !calls);
        fvm_value_undefined(args+1); assert(fvm_object_call(c, klass.object, "UTC", 8, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED && !calls);
        fvm_value_string(args, "010"); assert(fvm_construct(c, "Date", 1, args, &date) == FVM_OK); assert(fvm_object_call(c, date, "getTime", 0, 0, &out) == FVM_OK && out.number == (version >= 6 ? 8 : 10));
        fvm_value_undefined(args); calls = 0; assert(fvm_construct(c, "Date", 8, args, &date) == FVM_OK && !calls); assert(fvm_object_call(c, date, "getTime", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number > 0);
        args[0] = object(arg); assert(fvm_construct(c, "Date", 1, args, &date) == FVM_ERROR); assert(fvm_object_call(c, klass.object, "UTC", 1, args, &out) == FVM_ERROR);
        assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK); FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK); assert(fvm_channel_send_value(c, channel, &klass, 0) == FVM_OK); assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); assert(fvm_object_call(other, copy, "UTC", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED); fvm_value_string(&value, "010"); assert(fvm_function_construct(other, copy, 1, &value, &date) == FVM_OK); assert(fvm_object_call(other, date, "getTime", 0, 0, &out) == FVM_OK && out.number == (version >= 6 ? 8 : 10));
        assert(c->gc_frames == 0 && other->gc_frames == 0 && !c->primitive_depth && !other->primitive_depth); fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
