/* Loaded shift/pop preserve borrowed length and use raw own elements. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static unsigned writes;
static FVM_Result setter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++writes; fvm_value_undefined(out); return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *get_fn = 0, *set_fn = 0, *proto = 0;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_function_create_native(c, getter, 0, 0, &get_fn) == FVM_OK); assert(fvm_gc_pin(c, get_fn) == FVM_OK);
        assert(fvm_function_create_native(c, setter, 0, 0, &set_fn) == FVM_OK); assert(fvm_gc_pin(c, set_fn) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK); assert(fvm_gc_pin(c, proto) == FVM_OK);
        FVM_Value value, args[3], out; fvm_value_number(&value, 77); assert(fvm_object_set(c, proto, "1", &value) == FVM_OK);
        for (unsigned method = 0; method < 2; ++method) {
            FVM_Object *o = 0; assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
            assert(fvm_object_set_prototype(c, o, proto) == FVM_OK);
            fvm_value_number(&value, 2); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
            fvm_value_string_bytes(&value, "a\0b", 3); assert(fvm_object_set(c, o, "0", &value) == FVM_OK);
            fvm_value_string(args, "0"); args[1] = object(get_fn); args[2] = object(set_fn);
            assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
            assert(fvm_object_set_property_flags(c, o, "0", 2, 0) == FVM_OK);
            reads = writes = 0; assert(borrow(c, array, method ? "pop" : "shift", o, 0, 0, &out) == FVM_OK);
            assert(reads == 0 && writes == 0); number(c, o, "length", 2);
            if (method) assert(out.type == FVM_VALUE_UNDEFINED);
            else assert(out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "a\0b", 3));
            assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
            assert(borrow(c, array, "slice", o, 0, 0, &out) == FVM_OK);
            assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK);
            assert(value.type == (method ? FVM_VALUE_STRING : FVM_VALUE_UNDEFINED));
        }
        for (unsigned method = 0; method < 2; ++method) {
            FVM_Object *o = 0; fvm_value_number(&value, 2); assert(fvm_construct(c, "Array", 1, &value, &o) == FVM_OK);
            fvm_value_number(&value, 9); assert(fvm_object_set(c, o, "1", &value) == FVM_OK);
            assert(fvm_object_set_property_flags(c, o, "1", 2, 0) == FVM_OK);
            assert(fvm_object_call(c, o, method ? "pop" : "shift", 0, 0, &out) == FVM_OK);
            number(c, o, "length", 1); number(c, o, "1", 9);
            if (method) assert(out.type == FVM_VALUE_NUMBER && out.number == 9);
            else { assert(out.type == FVM_VALUE_UNDEFINED); number(c, o, "0", 9); }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
