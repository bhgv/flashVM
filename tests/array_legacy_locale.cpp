#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include "../src/standard/base_state.hpp"
static unsigned calls, mode;
static FVM_Result project(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *, FVM_Value *out, void *)
{ assert(!n); ++calls; if (mode == 0) return fvm_value_string_bytes(out, "N\0Q", 3); if (mode == 1) fvm_value_number(out, 81); else return FVM_ERROR; return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        for (unsigned before = 0; before < 2; ++before) {
            FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK); FVM_Value klass, proto, out, old, args[2], value; FVM_Object *arr = 0, *fn = 0; fvm_value_undefined(&old);
            unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
            if (before) { assert(fvm_standard_install(c) == FVM_OK); assert(fvm_get_global(c, "Array", &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK); assert(fvm_object_get(c, proto.object, "toLocaleString", &old) == FVM_OK && old.type == FVM_VALUE_OBJECT); assert(fvm_gc_pin(c, old.object) == FVM_OK); }
            if (version) assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK); assert(fvm_get_global(c, "Array", &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK); int own = 0; assert(fvm_object_has_own(c, proto.object, "toLocaleString", &own) == FVM_OK && !!own == !version); if (version && before) assert(proto.object->method_values.find("toLocaleString") == proto.object->method_values.end());
            fvm_value_number(args, 1); fvm_value_number(args+1, 2); assert(fvm_construct(c, "Array", 2, args, &arr) == FVM_OK); assert(fvm_gc_pin(c, arr) == FVM_OK); fvm_value_string(&value, "-"); assert(fvm_object_call(c, arr, "toLocaleString", 1, &value, &out) == FVM_OK && !std::strcmp(out.string, version ? "1,2" : "1-2")); if (before) { assert(fvm_function_invoke(c, old.object, arr, 1, &value, &out) == FVM_OK && !std::strcmp(out.string, "1-2")); }
            if (version) assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK); FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK); value = object(arr); assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK); assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); assert(fvm_object_call(other, copy, "toLocaleString", 0, 0, &out) == FVM_OK && !std::strcmp(out.string, "1,2"));
            assert(fvm_function_create_native(c, project, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, arr, "toString", &value) == FVM_OK); for (mode = 0; mode < 3; ++mode) { calls = 0; FVM_Result r = fvm_object_call(c, arr, "toLocaleString", 0, 0, &out); assert(r == (version && mode == 2 ? FVM_ERROR : FVM_OK)); assert(calls == (version ? 1U : 0U)); if (version && r == FVM_OK) { if (!mode) assert(fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "N\0Q", 3)); else assert(!std::strcmp(out.string, "[type Object]")); } }
            if (version) { FVM_Object *bare = 0; assert(fvm_array_create_like(other, 0, 0, &bare) == FVM_OK); assert(!bare->prototype); assert(fvm_function_create_native(other, project, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(other, bare, "toString", &value) == FVM_OK); mode = 0; calls = 0; assert(fvm_object_call(other, bare, "toLocaleString", 0, 0, &out) == FVM_OK && calls == 1 && fvm_value_string_size(&out) == 3); }
            assert(c->gc_frames == 0 && other->gc_frames == 0 && !c->string_depth && !other->string_depth); fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
        }
    }
    for (unsigned custom = 0; custom < 2; ++custom) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK); FVM_Value klass, proto, value; assert(fvm_get_global(c, "Array", &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK);
        if (custom) { fvm_value_number(&value, 81); assert(fvm_object_set(c, proto.object, "toLocaleString", &value) == FVM_OK); } else assert(fvm_object_set_property_flags(c, proto.object, "toLocaleString", 0, 7) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); int own = 0; assert(fvm_object_has_own(c, proto.object, "toLocaleString", &own) == FVM_OK && own); assert(fvm_object_get(c, proto.object, "toLocaleString", &value) == FVM_OK && (custom ? value.type == FVM_VALUE_NUMBER && value.number == 81 : value.type == FVM_VALUE_OBJECT)); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
