#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include "flashvm/property.h"
static void generic(FVM_Context *c, FVM_Object *proto)
{
    FVM_Value out; int own = 1; assert(proto->resource_kind == 30 && !proto->resource_result_valid);
    assert(fvm_object_has_own(c, proto, "length", &own) == FVM_OK && !own);
    assert(fvm_object_get(c, proto, "length", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_object_call(c, proto, "valueOf", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT && out.object == proto);
    assert(fvm_object_call(c, proto, "join", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_STRING && !fvm_value_string_size(&out));
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) for (unsigned before = 0; before < 2; ++before) {
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
        FVM_Value klass, value, out; FVM_Object *proto = 0, *host = 0;
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        if (before) {
            assert(fvm_standard_install(c) == FVM_OK); assert(fvm_get_global(c, "Array", &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK); proto = value.object; number(c, proto, "length", 0);
            fvm_value_number(&value, 2); assert(fvm_construct(c, "Array", 1, &value, &host) == FVM_OK); assert(fvm_gc_pin(c, host) == FVM_OK);
            const unsigned char invalid[] = {'F','W','S',8}; assert(fvm_load_memory(c, invalid, sizeof(invalid)) != FVM_OK && !c->swf_loaded); number(c, proto, "length", 0);
            fvm_value_string_bytes(&value, "x\0y", 3); assert(fvm_object_set(c, proto, "marker", &value) == FVM_OK);
        }
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        assert(fvm_get_global(c, "Array", &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK);
        if (before) assert(value.object == proto); else proto = value.object;
        generic(c, proto); assert(c->array_builtin_prototype == proto); assert(fvm_gc_pin(c, proto) == FVM_OK);
        if (before) { assert(fvm_object_get(c, proto, "marker", &value) == FVM_OK && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "x\0y", 3)); number(c, host, "length", 2); }
        assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK); generic(c, proto);
        FVM_Object *arr = 0, *result_proto = 0; fvm_value_number(&value, 2); assert(fvm_construct(c, "Array", 1, &value, &arr) == FVM_OK); assert(fvm_object_call(c, arr, "slice", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT); number(c, out.object, "length", 2); assert(fvm_object_get_prototype(c, out.object, &result_proto) == FVM_OK && result_proto == proto);
        assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK);
        FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK); value = object(proto);
        assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK); assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK);
        assert(fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); generic(other, copy);
        fvm_value_number(&value, 81); assert(fvm_object_call(other, copy, "push", 1, &value, &out) == FVM_OK && out.number == 1); number(other, copy, "length", 1); number(other, copy, "0", 81);
        assert(copy->resource_kind == 30); unsigned int flags = 99; assert(fvm_object_property_flags(other, copy, "length", &flags) == FVM_OK && flags == 0);
        assert(fvm_object_get(other, copy, "constructor", &klass) == FVM_OK && klass.type == FVM_VALUE_OBJECT);
        FVM_Object *constructed = 0; fvm_value_number(&value, 3); assert(fvm_function_construct(other, klass.object, 1, &value, &constructed) == FVM_OK); number(other, constructed, "length", 3); assert(constructed->resource_kind == 28);
        assert(fvm_object_get_prototype(other, constructed, &value.object) == FVM_OK && value.object == copy);
        assert(c->gc_frames == 0 && other->gc_frames == 0 && !c->primitive_depth && !other->primitive_depth);
        fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
