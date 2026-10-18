#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static void flags(FVM_Context *c, FVM_Object *o, const char *name, unsigned expected)
{ unsigned actual = 99; assert(fvm_object_property_flags(c, o, name, &actual) == FVM_OK && actual == expected); }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK); const char *classes[] = {"Object","Array","String"}, *methods[] = {"valueOf","push","charAt"};
    for (unsigned version = 4; version <= 8; ++version) for (unsigned before = 0; before < 2; ++before) for (unsigned custom = 0; custom <= before; ++custom) for (unsigned kind = 0; kind < 3; ++kind) {
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK); FVM_Value klass, value, out; FVM_Object *proto = 0; const char *static_name = kind == 0 ? "registerClass" : "fromCharCode";
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        if (before) { assert(fvm_standard_install(c) == FVM_OK); assert(fvm_get_global(c, classes[kind], &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK); proto = value.object; flags(c, proto, methods[kind], 1); if (kind != 1) flags(c, klass.object, static_name, 1); if (custom) { assert(fvm_object_set_property_flags(c, proto, methods[kind], 0, 7) == FVM_OK); if (kind != 1) assert(fvm_object_set_property_flags(c, klass.object, static_name, 0, 7) == FVM_OK); } }
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK); assert(fvm_get_global(c, classes[kind], &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK); if (before) assert(value.object == proto); else proto = value.object;
        flags(c, proto, methods[kind], custom ? 0 : 3); flags(c, proto, "toString", 3); int deleted = 1; assert(fvm_object_delete(c, proto, "toString", &deleted) == FVM_OK && !deleted); fvm_value_number(&value, 81); assert(fvm_object_set(c, proto, methods[kind], &value) == FVM_OK); assert(fvm_object_get(c, proto, methods[kind], &out) == FVM_OK && out.number == 81); assert(fvm_object_delete(c, proto, methods[kind], &deleted) == FVM_OK && !!deleted == !!custom);
        if (kind != 1) { unsigned expected = custom ? 0 : kind == 0 ? 7 : 3; flags(c, klass.object, static_name, expected); assert(fvm_object_delete(c, klass.object, static_name, &deleted) == FVM_OK && !!deleted == !!custom); if (!custom) { assert(fvm_object_set(c, klass.object, static_name, &value) == FVM_OK); assert(fvm_object_get(c, klass.object, static_name, &out) == FVM_OK && out.type == (kind == 0 ? FVM_VALUE_OBJECT : FVM_VALUE_NUMBER)); } }
        assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK); FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK); assert(fvm_channel_send_value(c, channel, &klass, 0) == FVM_OK); assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); assert(fvm_object_get(other, copy, "prototype", &out) == FVM_OK); flags(other, out.object, "toString", 3); assert(fvm_object_delete(other, out.object, "toString", &deleted) == FVM_OK && !deleted);
        assert(c->gc_frames == 0 && other->gc_frames == 0 && !c->primitive_depth && !other->primitive_depth); fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
