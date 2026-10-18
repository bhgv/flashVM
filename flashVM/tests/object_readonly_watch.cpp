/* Loaded watchers precede readonly checks, can change flags, and run once on indexed setters. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
static unsigned mode, watched, writes;
static FVM_Result watcher(FVM_Context *c, FVM_Object *o, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    assert(n == 4 && a[1].number == 9 && a[2].number == 81); ++watched;
    if (mode == 1) assert(fvm_object_set_property_flags_bytes(c, o, a[0].string, fvm_value_string_size(a), 0, 4) == FVM_OK);
    if (mode == 2) assert(fvm_object_set_property_flags_bytes(c, o, a[0].string, fvm_value_string_size(a), 4, 0) == FVM_OK);
    fvm_value_number(out, 82); return FVM_OK;
}
static FVM_Result setter(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{ assert(n == 1 && a[0].number == 82); ++writes; fvm_value_undefined(out); return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 6) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *fn = 0, *get_fn = 0, *set_fn = 0, *array = 0;
        assert(fvm_function_create_native(c, watcher, 0, 0, &fn) == FVM_OK); assert(fvm_gc_pin(c, fn) == FVM_OK);
        assert(fvm_function_create_native(c, getter, 0, 0, &get_fn) == FVM_OK); assert(fvm_gc_pin(c, get_fn) == FVM_OK);
        assert(fvm_function_create_native(c, setter, 0, 0, &set_fn) == FVM_OK); assert(fvm_gc_pin(c, set_fn) == FVM_OK);
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
        for (unsigned kind = 0; kind < 4; ++kind)
        for (mode = 0; mode < 3; ++mode) {
            if (!version && kind == 2) continue;
            const char nul[] = {'n',0,'m'}; const char *key = kind == 3 ? nul : kind == 0 ? "x" : "0";
            size_t size = kind == 3 ? 3 : 1;
            FVM_Object *o = 0; FVM_Value value, args[3], out; assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
            fvm_value_number(&value, 1); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
            fvm_value_number(&value, 9); assert(fvm_object_set_bytes(c, o, key, size, &value) == FVM_OK);
            if (kind == 2) {
                fvm_value_string(args, "0"); args[1] = object(get_fn); args[2] = object(set_fn);
                assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
            }
            assert(fvm_object_set_property_flags_bytes(c, o, key, size, mode == 2 ? 0 : 4, 0) == FVM_OK);
            fvm_value_string_bytes(args, key, size); args[1] = object(fn);
            assert(fvm_object_call(c, o, "watch", 2, args, &out) == FVM_OK && out.boolean);
            watched = writes = reads = 0; fvm_value_number(&value, 81);
            assert(fvm_object_set_bytes(c, o, key, size, &value) == FVM_OK);
            assert(watched == (version || mode == 2 ? 1U : 0U));
            assert(writes == (kind == 2 ? 1U : 0U) && reads == 0);
            if (kind == 2) {
                assert(borrow(c, array, "slice", o, 0, 0, &out) == FVM_OK);
                assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK);
            } else assert(fvm_object_get_bytes(c, o, key, size, &value) == FVM_OK);
            assert(value.type == FVM_VALUE_NUMBER && value.number == (version ? mode == 1 ? 82 : 9 : mode == 2 ? 82 : 9));
            number(c, o, "length", 1);
            assert(c->gc_frames == 0 && c->calling_setter_name == 0 && c->calling_setter_owner == 0 && c->watch_running.empty());
            assert(fvm_gc_collect(c, 0) == FVM_OK);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
