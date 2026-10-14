#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include "flashvm/serialization.h"
#include "../src/standard/base_state.hpp"
static unsigned calls, mode;
static FVM_Result project(FVM_Context *, FVM_Object *self, unsigned n, const FVM_Value *, FVM_Value *out, void *)
{ assert(!n); ++calls; if (mode == 0) return fvm_value_string_bytes(out, "N\0Q", 3); if (mode == 1) fvm_value_number(out, 81); else if (mode == 2) *out = object(self); else if (mode == 3) fvm_value_undefined(out); else if (mode == 4) return FVM_ERROR; return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK); unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; if (version) { assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK); } assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value klass, proto, value, out, arg; FVM_Object *receiver = 0, *fn = 0; assert(fvm_get_global(c, "Object", &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK); assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK); assert(fvm_gc_pin(c, receiver) == FVM_OK); assert(fvm_function_create_native(c, project, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, receiver, "toString", &value) == FVM_OK); arg = value;
        for (mode = 0; mode < 5; ++mode) { calls = 0; FVM_Result r = borrow(c, proto.object, "toLocaleString", receiver, 1, &arg, &out); assert(r == (version && mode == 4 ? FVM_ERROR : FVM_OK)); assert(calls == (version ? 1U : 0U)); if (r == FVM_OK) { if (!version) assert(!std::strcmp(out.string, "[object Object]")); else if (!mode) assert(fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "N\0Q", 3)); else assert(!std::strcmp(out.string, "[type Object]")); } assert(c->gc_frames == 0 && !c->string_depth && !c->primitive_depth); }
        assert(borrow(c, proto.object, "toString", fn, 0, 0, &out) == FVM_OK && !std::strcmp(out.string, version ? "[type Function]" : "[object Object]"));
        fvm_value_number(&value, 81); assert(fvm_object_set(c, receiver, "toString", &value) == FVM_OK); assert(borrow(c, proto.object, "toLocaleString", receiver, 0, 0, &out) == FVM_OK && !std::strcmp(out.string, version ? "[type Object]" : "[object Object]"));
        assert(fvm_object_get(c, proto.object, "toLocaleString", &value) == FVM_OK); unsigned char *data = 0; size_t size = 0; assert(fvm_value_serialize(c, &value, &data, &size) == FVM_OK); assert(fvm_value_deserialize(other, data, size, &out) == FVM_OK); fvm_serialized_free(data); FVM_Object *locale = out.object; assert(fvm_gc_pin(other, locale) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); FVM_Object *plain = 0; assert(fvm_object_create(other, &plain) == FVM_OK); assert(fvm_function_invoke(other, locale, plain, 0, 0, &out) == FVM_OK && !std::strcmp(out.string, version ? "[type Object]" : "[object Object]"));
        assert(fvm_base_callback_from_id(612) == fvm_base_callback_from_id(611)); assert(fvm_base_callback_id(fvm_base_callback_from_id(627)) == 627);
        if (version) { FVM_Value recursion; assert(fvm_object_get(c, proto.object, "toLocaleString", &recursion) == FVM_OK); assert(fvm_object_set(c, receiver, "toString", &recursion) == FVM_OK); assert(borrow(c, proto.object, "toLocaleString", receiver, 0, 0, &out) == FVM_ERROR && !c->string_depth && !c->primitive_depth && c->gc_frames == 0); }
        fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
