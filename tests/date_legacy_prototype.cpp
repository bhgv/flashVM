#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static unsigned calls;
static FVM_Result operand(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++calls; fvm_value_number(out, 2); return FVM_OK; }
static FVM_Result failure(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{ return FVM_ERROR; }
static FVM_Result borrow_wide(FVM_Context *c, FVM_Object *proto, const char *name, FVM_Object *receiver, unsigned n, const FVM_Value *args, FVM_Value *out)
{
    FVM_Value method, call[9]; assert(n <= 8); assert(fvm_object_get(c, proto, name, &method) == FVM_OK && method.type == FVM_VALUE_OBJECT); call[0] = object(receiver);
    for (unsigned i = 0; i < n; ++i) call[i+1] = args[i];
    return fvm_object_call(c, method.object, "call", n+1, call, out);
}
static void generic(FVM_Context *c, FVM_Object *proto)
{
    const char *names[] = {"getTime","valueOf","toString","getFullYear","setTime","setUTCMonth"}; FVM_Value out;
    assert(proto->resource_kind == 30 && !proto->resource_result_valid);
    for (unsigned i = 0; i < 6; ++i) assert(fvm_object_call(c, proto, names[i], 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) for (unsigned before = 0; before < 2; ++before) {
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
        FVM_Value klass, value, out, args[8]; FVM_Object *proto = 0, *host = 0, *plain = 0, *arg = 0, *fn = 0;
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        if (before) {
            assert(fvm_standard_install(c) == FVM_OK); assert(fvm_get_global(c, "Date", &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK); proto = value.object;
            assert(fvm_object_call(c, proto, "valueOf", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number != out.number);
            fvm_value_number(&value, 81); assert(fvm_construct(c, "Date", 1, &value, &host) == FVM_OK); assert(fvm_gc_pin(c, host) == FVM_OK);
            const unsigned char invalid[] = {'F','W','S',8}; assert(fvm_load_memory(c, invalid, sizeof(invalid)) != FVM_OK && !c->swf_loaded);
            fvm_value_string_bytes(&value, "x\0y", 3); assert(fvm_object_set(c, proto, "marker", &value) == FVM_OK);
        }
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        assert(fvm_get_global(c, "Date", &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK); if (before) assert(value.object == proto); else proto = value.object;
        generic(c, proto); assert(fvm_gc_pin(c, proto) == FVM_OK); if (before) { assert(fvm_object_get(c, proto, "marker", &value) == FVM_OK && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "x\0y", 3)); assert(fvm_object_call(c, host, "getTime", 0, 0, &out) == FVM_OK && out.number == 81); }
        assert(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK); assert(fvm_construct(c, "Object", 0, 0, &arg) == FVM_OK);
        assert(fvm_function_create_native(c, operand, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, arg, "valueOf", &value) == FVM_OK);
        for (unsigned i = 0; i < 8; ++i) args[i] = object(arg);
        const char *methods[] = {"getTime","toString","getFullYear","getTimezoneOffset","setTime","setUTCMonth"};
        for (unsigned i = 0; i < 6; ++i) { calls = 0; assert(borrow_wide(c, proto, methods[i], plain, 8, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED && calls == 7); }
        FVM_Object *probe = 0; fvm_value_number(&value, 81); assert(fvm_construct(c, "Date", 1, &value, &probe) == FVM_OK); calls = 0; assert(fvm_object_call(c, probe, "getTime", 8, args, &out) == FVM_OK && out.number == 81 && calls == 7);
        fvm_value_undefined(args+2); calls = 0; assert(borrow_wide(c, proto, "getTime", plain, 8, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED && calls == 2);
        assert(fvm_function_create_native(c, failure, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, arg, "valueOf", &value) == FVM_OK); assert(borrow_wide(c, proto, "getTime", plain, 8, args, &out) == FVM_ERROR); assert(fvm_object_call(c, probe, "setTime", 8, args, &out) == FVM_ERROR); assert(fvm_object_call(c, probe, "getTime", 0, 0, &out) == FVM_OK && out.number == 81);
        FVM_Object *valid = 0; fvm_value_number(&value, 0); assert(fvm_construct(c, "Date", 1, &value, &valid) == FVM_OK); fvm_value_string(&value, "010"); assert(fvm_object_call(c, valid, "setTime", 1, &value, &out) == FVM_OK && out.number == (version >= 6 ? 8 : 10));
        assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK); generic(c, proto);
        assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK); FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK); value = object(proto); assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK); assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK); assert(fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); generic(other, copy);
        assert(fvm_object_get(other, copy, "constructor", &klass) == FVM_OK); FVM_Object *constructed = 0; fvm_value_number(&value, 81); assert(fvm_function_construct(other, klass.object, 1, &value, &constructed) == FVM_OK); assert(fvm_object_call(other, constructed, "getTime", 0, 0, &out) == FVM_OK && out.number == 81); assert(fvm_object_get_prototype(other, constructed, &value.object) == FVM_OK && value.object == copy);
        assert(c->gc_frames == 0 && other->gc_frames == 0 && !c->primitive_depth && !other->primitive_depth); fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
