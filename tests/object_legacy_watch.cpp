/* Loaded watcher names coerce before validation; old values use raw stored data. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static unsigned watched, writes, converted;
static size_t key_size;
static FVM_Result watch_callback(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    assert(n == 4 && a[0].type == FVM_VALUE_STRING && fvm_value_string_size(a) == key_size);
    assert(a[1].type == FVM_VALUE_NUMBER && a[1].number == 9);
    assert(a[2].type == FVM_VALUE_NUMBER && a[2].number == 81);
    ++watched; fvm_value_number(out, 82); return FVM_OK;
}
static FVM_Result setter(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{ assert(n == 1 && a[0].number == 82); ++writes; fvm_value_undefined(out); return FVM_OK; }
static FVM_Result name(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++converted; return fvm_value_string(out, "1"); }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *o = 0, *array = 0, *get_fn = 0, *set_fn = 0, *watch_fn = 0, *key = 0;
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_function_create_native(c, getter, 0, 0, &get_fn) == FVM_OK);
        assert(fvm_function_create_native(c, setter, 0, 0, &set_fn) == FVM_OK);
        assert(fvm_function_create_native(c, watch_callback, 0, 0, &watch_fn) == FVM_OK); assert(fvm_gc_pin(c, watch_fn) == FVM_OK);
        FVM_Value value, args[3], out; fvm_value_number(&value, 2); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_object_set(c, o, "1", &value) == FVM_OK);
        fvm_value_string(args, "1"); args[1] = object(get_fn); args[2] = object(set_fn);
        assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        fvm_value_number(args, 1); args[1] = object(watch_fn);
        assert(fvm_object_call(c, o, "watch", 2, args, &out) == FVM_OK && out.boolean);
        key_size = 1; reads = writes = watched = 0; fvm_value_number(&value, 81);
        assert(fvm_object_set(c, o, "1", &value) == FVM_OK && watched == 1 && writes == 1 && reads == 0);
        assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(borrow(c, array, "slice", o, 0, 0, &out) == FVM_OK); number(c, out.object, "1", 82);
        assert(fvm_construct(c, "Object", 0, 0, &key) == FVM_OK); assert(fvm_gc_pin(c, key) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, key, "toString", name, 0, 0) == FVM_OK);
        args[0] = object(key); converted = 0;
        assert(fvm_object_call(c, o, "unwatch", 1, args, &out) == FVM_OK && out.boolean && converted == 1);
        assert(fvm_object_call(c, o, "unwatch", 1, args, &out) == FVM_OK && !out.boolean && converted == 2);
        converted = 0; assert(fvm_object_call(c, o, "watch", 1, args, &out) == FVM_OK && !out.boolean && converted == 1);
        fvm_value_string(args, ""); args[1] = object(watch_fn);
        assert(fvm_object_call(c, o, "watch", 2, args, &out) == FVM_OK && out.boolean);
        watched = 0; assert(fvm_object_set(c, o, "", &value) == FVM_OK && watched == 0);
        assert(fvm_object_call(c, o, "unwatch", 1, args, &out) == FVM_OK && out.boolean);
        const char nul[] = {'n',0,'m'}; fvm_value_number(&value, 9); assert(fvm_object_set_bytes(c, o, nul, sizeof(nul), &value) == FVM_OK);
        fvm_value_string_bytes(args, nul, sizeof(nul)); args[1] = object(watch_fn);
        assert(fvm_object_call(c, o, "watch", 2, args, &out) == FVM_OK && out.boolean);
        watched = 0; key_size = 3; fvm_value_number(&value, 81);
        assert(fvm_object_set_bytes(c, o, nul, sizeof(nul), &value) == FVM_OK && watched == 1);
        assert(fvm_object_get_bytes(c, o, nul, sizeof(nul), &out) == FVM_OK && out.number == 82);
        FVM_Object *parent = 0, *child = 0;
        assert(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK);
        assert(fvm_gc_pin(c, parent) == FVM_OK);
        fvm_value_number(&value, 2); assert(fvm_object_set(c, parent, "length", &value) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_object_set(c, parent, "1", &value) == FVM_OK);
        fvm_value_string(args, "1"); args[1] = object(get_fn); args[2] = object(set_fn);
        assert(fvm_object_call(c, parent, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        assert(fvm_construct(c, "Object", 0, 0, &child) == FVM_OK); assert(fvm_object_set_prototype(c, child, parent) == FVM_OK);
        args[1] = object(watch_fn); assert(fvm_object_call(c, child, "watch", 2, args, &out) == FVM_OK && out.boolean);
        key_size = 1; reads = watched = writes = 0; fvm_value_number(&value, 81);
        assert(fvm_object_set(c, child, "1", &value) == FVM_OK && reads == 0 && watched == 1 && writes == 1);
        int own = 0; assert(fvm_object_has_own(c, child, "1", &own) == FVM_OK && !own);
        assert(borrow(c, array, "slice", parent, 0, 0, &out) == FVM_OK); number(c, out.object, "1", 9);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
