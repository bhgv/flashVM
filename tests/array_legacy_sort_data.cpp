/* Loaded generic sort reads own retained data and writes without index getters/setters/watchers. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static unsigned writes, watched;
static FVM_Result setter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++writes; fvm_value_undefined(out); return FVM_OK; }
static FVM_Result watcher(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{ assert(n == 4); ++watched; *out = a[2]; return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *get_fn = 0, *set_fn = 0, *watch_fn = 0, *proto = 0;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_function_create_native(c, getter, 0, 0, &get_fn) == FVM_OK); assert(fvm_gc_pin(c, get_fn) == FVM_OK);
        assert(fvm_function_create_native(c, setter, 0, 0, &set_fn) == FVM_OK); assert(fvm_gc_pin(c, set_fn) == FVM_OK);
        assert(fvm_function_create_native(c, watcher, 0, 0, &watch_fn) == FVM_OK); assert(fvm_gc_pin(c, watch_fn) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK); assert(fvm_gc_pin(c, proto) == FVM_OK);
        FVM_Value value, args[3], out; fvm_value_number(&value, 77); assert(fvm_object_set(c, proto, "1", &value) == FVM_OK);
        for (unsigned readonly = 0; readonly < 2; ++readonly) {
            FVM_Object *o = 0; assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
            assert(fvm_object_set_prototype(c, o, proto) == FVM_OK);
            fvm_value_number(&value, 4); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
            fvm_value_number(&value, 9); assert(fvm_object_set(c, o, "0", &value) == FVM_OK);
            fvm_value_number(&value, 1); assert(fvm_object_set(c, o, "2", &value) == FVM_OK);
            fvm_value_number(&value, 8); assert(fvm_object_set(c, o, "3", &value) == FVM_OK);
            args[1] = object(get_fn); args[2] = object(set_fn); fvm_value_string(args, "length");
            assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
            fvm_value_string(args, "0"); assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
            if (readonly) assert(fvm_object_set_property_flags(c, o, "0", 4, 0) == FVM_OK);
            args[1] = object(watch_fn); assert(fvm_object_call(c, o, "watch", 2, args, &out) == FVM_OK && out.boolean);
            reads = writes = watched = 0; assert(borrow(c, array, "sort", o, 0, 0, &out) == FVM_OK && out.object == o && reads == 0 && writes == 0 && watched == 0);
            assert(fvm_gc_collect(c, 0) == FVM_OK); assert(borrow(c, array, "slice", o, 0, 0, &out) == FVM_OK);
            number(c, out.object, "length", 4); number(c, out.object, "0", readonly ? 9 : 1); number(c, out.object, "1", 8); number(c, out.object, "2", 9);
            assert(fvm_table_get_index(c, out.object, 3, &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
