#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static void flags(FVM_Context *c, FVM_Object *o, const char *name, unsigned expected)
{ unsigned actual = 99; assert(fvm_object_property_flags(c, o, name, &actual) == FVM_OK && actual == expected); }

int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK); const char *classes[] = {"Number","Boolean"};
    for (unsigned version = 4; version <= 8; ++version) for (unsigned before = 0; before < 2; ++before) for (unsigned custom = 0; custom <= before; ++custom) for (unsigned kind = 0; kind < 2; ++kind) {
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK); FVM_Value klass, value, out; FVM_Object *proto = 0;
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        if (before) { assert(fvm_standard_install(c) == FVM_OK); assert(fvm_get_global(c, classes[kind], &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK); proto = value.object; flags(c, proto, "valueOf", 1); if (!kind) flags(c, klass.object, "MAX_VALUE", 0); if (custom) { assert(fvm_object_set_property_flags(c, proto, "valueOf", 0, 7) == FVM_OK); if (!kind) { fvm_value_number(&value, 81); assert(fvm_object_set(c, klass.object, "MAX_VALUE", &value) == FVM_OK); assert(fvm_object_set_property_flags(c, klass.object, "MIN_VALUE", 2, 5) == FVM_OK); } } }
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK); assert(fvm_get_global(c, classes[kind], &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK); if (before) assert(value.object == proto); else proto = value.object;
        flags(c, proto, "valueOf", custom ? 0 : 3); flags(c, proto, "toString", 3); int deleted = 1; assert(fvm_object_delete(c, proto, "toString", &deleted) == FVM_OK && !deleted); fvm_value_number(&value, 81); assert(fvm_object_set(c, proto, "valueOf", &value) == FVM_OK); assert(fvm_object_get(c, proto, "valueOf", &out) == FVM_OK && out.number == 81); assert(fvm_object_delete(c, proto, "valueOf", &deleted) == FVM_OK && !!deleted == !!custom);
        if (!kind) { const char *constants[] = {"MAX_VALUE","MIN_VALUE","NaN","NEGATIVE_INFINITY","POSITIVE_INFINITY"}; for (unsigned i = 0; i < 5; ++i) flags(c, klass.object, constants[i], custom && i < 2 ? (i ? 2 : 0) : 7); if (custom) assert(fvm_object_get(c, klass.object, "MAX_VALUE", &out) == FVM_OK && out.number == 81); assert(fvm_object_set(c, klass.object, "NaN", &value) == FVM_OK); assert(fvm_object_get(c, klass.object, "NaN", &out) == FVM_OK && out.number != out.number); assert(fvm_object_delete(c, klass.object, "NaN", &deleted) == FVM_OK && !deleted); }
        assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK); FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK); assert(fvm_channel_send_value(c, channel, &klass, 0) == FVM_OK); assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); assert(fvm_object_get(other, copy, "prototype", &out) == FVM_OK); flags(other, out.object, "toString", 3); assert(fvm_object_delete(other, out.object, "toString", &deleted) == FVM_OK && !deleted); if (!kind) flags(other, copy, "NaN", 7);
        assert(c->gc_frames == 0 && other->gc_frames == 0 && !c->primitive_depth && !other->primitive_depth); fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
