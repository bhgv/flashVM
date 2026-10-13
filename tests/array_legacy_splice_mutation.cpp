/* Loaded generic splice uses own presence/raw data and leaves borrowed tails beyond new length. */
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
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK); assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        FVM_Object *array = 0, *get_fn = 0, *set_fn = 0, *watch_fn = 0, *proto = 0;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_function_create_native(c, getter, 0, 0, &get_fn) == FVM_OK); assert(fvm_gc_pin(c, get_fn) == FVM_OK);
        assert(fvm_function_create_native(c, setter, 0, 0, &set_fn) == FVM_OK); assert(fvm_gc_pin(c, set_fn) == FVM_OK);
        assert(fvm_function_create_native(c, watcher, 0, 0, &watch_fn) == FVM_OK); assert(fvm_gc_pin(c, watch_fn) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK); assert(fvm_gc_pin(c, proto) == FVM_OK);
        FVM_Value value, args[3], out; fvm_value_number(&value, 77); assert(fvm_object_set(c, proto, "2", &value) == FVM_OK);
        for (unsigned mode = 0; mode < 3; ++mode) {
            FVM_Object *o = 0; assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
            assert(fvm_object_set_prototype(c, o, proto) == FVM_OK);
            fvm_value_number(&value, 4); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
            for (unsigned i = 0; i < 4; ++i) { if (mode == 2 && i == 2) continue; fvm_value_number(&value, 9 - i); assert(fvm_table_set_index(c, o, i, &value) == FVM_OK); }
            args[1] = object(get_fn); args[2] = object(set_fn); fvm_value_string(args, "length");
            assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
            fvm_value_string(args, "1"); assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
            if (mode != 2) { fvm_value_string(args, "2"); assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean); assert(fvm_object_set_property_flags(c, o, "2", 2, 0) == FVM_OK); }
            if (mode == 1) assert(fvm_object_set_property_flags(c, o, "1", 6, 0) == FVM_OK);
            fvm_value_string(args, "1"); args[1] = object(watch_fn); assert(fvm_object_call(c, o, "watch", 2, args, &out) == FVM_OK && out.boolean);
            fvm_value_number(args, 1); fvm_value_number(args + 1, 1); reads = writes = watched = 0;
            assert(borrow(c, array, "splice", o, 2, args, &out) == FVM_OK && reads == 0 && writes == 0 && watched == 0);
            number(c, out.object, "0", 8); number(c, out.object, "length", 1);
            int own = 0; assert(fvm_object_has_own(c, o, "3", &own) == FVM_OK && own);
            assert(fvm_gc_collect(c, 0) == FVM_OK); assert(borrow(c, array, "slice", o, 0, 0, &out) == FVM_OK); number(c, out.object, "length", 3);
            assert(fvm_table_get_index(c, out.object, 1, &value) == FVM_OK);
            if (mode == 2) assert(value.type == FVM_VALUE_UNDEFINED);
            else assert(value.type == FVM_VALUE_NUMBER && value.number == (mode == 1 ? 8 : 7));
            number(c, out.object, "2", 6);
        }
        FVM_Object *o = 0; assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
        fvm_value_number(&value, 1); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        fvm_value_string_bytes(&value, "a\0b", 3); assert(fvm_object_set(c, o, "0", &value) == FVM_OK);
        fvm_value_number(args, 0); fvm_value_number(args + 1, 1); assert(borrow(c, array, "splice", o, 2, args, &out) == FVM_OK);
        number(c, o, "length", 0); assert(fvm_object_get(c, o, "0", &value) == FVM_OK && value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3);
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
        fvm_value_number(&value, 2); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        fvm_value_string(&value, "old"); assert(fvm_object_set(c, o, "0", &value) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_object_set(c, o, "1", &value) == FVM_OK);
        fvm_value_string(args, "1"); args[1] = object(get_fn); args[2] = object(set_fn);
        assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        assert(fvm_object_set_property_flags(c, o, "1", 2, 0) == FVM_OK);
        fvm_value_number(args, 0); fvm_value_number(args + 1, 0); fvm_value_string_bytes(args + 2, "a\0b", 3);
        reads = writes = watched = 0; assert(borrow(c, array, "splice", o, 3, args, &out) == FVM_OK && reads == 0 && writes == 0 && watched == 0);
        number(c, out.object, "length", 0); assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(borrow(c, array, "slice", o, 0, 0, &out) == FVM_OK); number(c, out.object, "length", 3); number(c, out.object, "2", 9);
        assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK && value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
        assert(fvm_table_get_index(c, out.object, 1, &value) == FVM_OK && value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "old"));
        fvm_value_number(&value, 4); assert(fvm_construct(c, "Array", 1, &value, &o) == FVM_OK);
        assert(fvm_object_set_prototype(c, o, proto) == FVM_OK); fvm_value_number(&value, 6); assert(fvm_object_set(c, o, "3", &value) == FVM_OK);
        assert(fvm_object_set_property_flags(c, o, "3", 2, 0) == FVM_OK);
        fvm_value_number(args, 1); fvm_value_number(args + 1, 1); assert(borrow(c, array, "splice", o, 2, args, &out) == FVM_OK);
        number(c, o, "length", 3); number(c, o, "3", 6); number(c, o, "2", 6);
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
        fvm_value_number(&value, 3); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        for (unsigned i = 0; i < 3; ++i) { fvm_value_number(&value, 9 - i); assert(fvm_table_set_index(c, o, i, &value) == FVM_OK); }
        fvm_value_number(args, 0); fvm_value_number(args + 1, 1);
        assert(borrow(c, array, "splice", o, 2, args, &out) == FVM_OK);
        number(c, o, "length", 2); number(c, o, "0", 8); number(c, o, "1", 7); number(c, o, "2", 7);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
