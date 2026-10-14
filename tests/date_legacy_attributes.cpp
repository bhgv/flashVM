#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static void flags(FVM_Context *c, FVM_Object *o, const char *name, unsigned expected)
{ unsigned actual = 99; assert(fvm_object_property_flags(c, o, name, &actual) == FVM_OK && actual == expected); }

int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) for (unsigned before = 0; before < 2; ++before) for (unsigned custom = 0; custom <= before; ++custom) {
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
        FVM_Value klass, value, out, args[2]; FVM_Object *proto = 0;
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        if (before) {
            assert(fvm_standard_install(c) == FVM_OK); assert(fvm_get_global(c, "Date", &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK); proto = value.object; flags(c, proto, "getTime", 1); flags(c, klass.object, "UTC", 1);
            if (custom) { assert(fvm_object_set_property_flags(c, proto, "getTime", 0, 7) == FVM_OK); assert(fvm_object_set_property_flags(c, klass.object, "UTC", 0, 7) == FVM_OK); }
        }
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK); assert(fvm_get_global(c, "Date", &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK); if (before) assert(value.object == proto); else proto = value.object;
        flags(c, proto, "getTime", custom ? 0 : 3); flags(c, proto, "toString", 3); flags(c, klass.object, "UTC", custom ? 0 : 7);
        int deleted = 1; assert(fvm_object_delete(c, proto, "toString", &deleted) == FVM_OK && !deleted);
        fvm_value_number(&value, 81); assert(fvm_object_set(c, proto, "getTime", &value) == FVM_OK); assert(fvm_object_get(c, proto, "getTime", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 81); flags(c, proto, "getTime", custom ? 0 : 3);
        assert(fvm_object_delete(c, proto, "getTime", &deleted) == FVM_OK && !!deleted == !!custom);
        if (!custom) { assert(fvm_object_set(c, klass.object, "UTC", &value) == FVM_OK); assert(fvm_object_get(c, klass.object, "UTC", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT); assert(fvm_object_delete(c, klass.object, "UTC", &deleted) == FVM_OK && !deleted); fvm_value_number(args, 1970); fvm_value_number(args+1, 0); assert(fvm_object_call(c, klass.object, "UTC", 2, args, &out) == FVM_OK && out.number == 0); }
        assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK); FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK); assert(fvm_channel_send_value(c, channel, &klass, 0) == FVM_OK); assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK); assert(fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); flags(other, copy, "UTC", custom ? 0 : 7); assert(fvm_object_get(other, copy, "prototype", &out) == FVM_OK); flags(other, out.object, "toString", 3); assert(fvm_object_delete(other, out.object, "toString", &deleted) == FVM_OK && !deleted);
        assert(c->gc_frames == 0 && other->gc_frames == 0 && !c->primitive_depth && !other->primitive_depth); fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
