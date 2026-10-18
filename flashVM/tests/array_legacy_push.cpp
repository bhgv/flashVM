/* Loaded push preserves virtual descriptors, bypasses callbacks and updates raw own length. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "../src/standard/base_state.hpp"
#include "flashvm/gc.h"
static unsigned writes, watched;
static FVM_Result setter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++writes; fvm_value_undefined(out); return FVM_OK; }
static FVM_Result watcher(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *args, FVM_Value *out, void *)
{ assert(n == 4); ++watched; *out = args[2]; return FVM_OK; }
static double data(FVM_Object *o, const char *name)
{
    FVM_Value value; assert(fvm_base_accessor_data(o->accessors.find(name)->second, value));
    assert(value.type == FVM_VALUE_NUMBER); return value.number;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *get_fn = 0, *set_fn = 0, *watch_fn = 0;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_function_create_native(c, getter, 0, 0, &get_fn) == FVM_OK); assert(fvm_gc_pin(c, get_fn) == FVM_OK);
        assert(fvm_function_create_native(c, setter, 0, 0, &set_fn) == FVM_OK); assert(fvm_gc_pin(c, set_fn) == FVM_OK);
        assert(fvm_function_create_native(c, watcher, 0, 0, &watch_fn) == FVM_OK); assert(fvm_gc_pin(c, watch_fn) == FVM_OK);
        for (unsigned protect = 0; protect < 3; ++protect) {
            FVM_Object *o = 0; FVM_Value value, args[3], out; assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
            fvm_value_number(&value, 2); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
            fvm_value_number(&value, 9); assert(fvm_object_set(c, o, "2", &value) == FVM_OK);
            args[1] = object(get_fn); args[2] = object(set_fn); fvm_value_string(args, "length");
            assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
            fvm_value_string(args, "2"); assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
            if (protect) assert(fvm_object_set_property_flags(c, o, protect == 1 ? "length" : "2", 4, 0) == FVM_OK);
            args[1] = object(watch_fn); assert(fvm_object_call(c, o, "watch", 2, args, &out) == FVM_OK && out.boolean);
            reads = writes = watched = 0; fvm_value_string_bytes(&value, "a\0b", 3);
            assert(borrow(c, array, "push", o, 1, &value, &out) == FVM_OK && out.number == 3 && reads == 0 && writes == 0 && watched == 0);
            assert(data(o, "length") == (protect == 1 ? 2 : 3));
            assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_base_accessor_data(o->accessors.find("2")->second, value));
            if (protect == 2) assert(value.type == FVM_VALUE_NUMBER && value.number == 9);
            else assert(value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
        }
        FVM_Value value, out; FVM_Object *o = 0; fvm_value_number(&value, 2);
        assert(fvm_construct(c, "Array", 1, &value, &o) == FVM_OK);
        assert(fvm_object_set_property_flags(c, o, "length", 4, 0) == FVM_OK); fvm_value_number(&value, 81);
        assert(fvm_object_call(c, o, "push", 1, &value, &out) == FVM_OK && out.number == 3); number(c, o, "length", 2); number(c, o, "2", 81);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
