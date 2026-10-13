/* Loaded native splice handles live source flags and skips fast-path watchers. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static unsigned watched;
static FVM_Result watcher(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *, FVM_Value *out, void *)
{ assert(n == 4); ++watched; fvm_value_number(out, 82); return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value values[3], args[3], out; FVM_Object *o = 0, *fn = 0;
        for (unsigned i = 0; i < 3; ++i) fvm_value_number(values + i, 9 - i);
        for (unsigned bits = 0; bits < 8; ++bits) {
            assert(fvm_construct(c, "Array", 3, values, &o) == FVM_OK);
            assert(fvm_object_set_property_flags(c, o, "1", bits, 0) == FVM_OK);
            fvm_value_number(args, 0); fvm_value_number(args + 1, 1);
            assert(fvm_object_call(c, o, "splice", 2, args, &out) == FVM_OK);
            number(c, o, "length", 2); number(c, o, "0", 8); number(c, o, "1", (bits & 6) == 6 ? 8 : 7);
            unsigned flags = 0; assert(fvm_object_property_flags(c, o, "1", &flags) == FVM_OK && flags == (bits & 2 ? bits : 0));
        }
        assert(fvm_construct(c, "Array", 3, values, &o) == FVM_OK);
        assert(fvm_object_set_property_flags(c, o, "2", 2, 0) == FVM_OK);
        assert(fvm_object_call(c, o, "splice", 2, args, &out) == FVM_OK);
        number(c, o, "length", 2); number(c, o, "2", 7);
        assert(fvm_construct(c, "Array", 3, values, &o) == FVM_OK);
        assert(fvm_object_set_property_flags(c, o, "length", 4, 0) == FVM_OK);
        fvm_value_number(args + 1, 0); fvm_value_number(args + 2, 81);
        assert(fvm_object_call(c, o, "splice", 3, args, &out) == FVM_OK);
        number(c, o, "length", 3); number(c, o, "0", 81); number(c, o, "3", 7);
        assert(fvm_construct(c, "Array", 3, values, &o) == FVM_OK);
        assert(fvm_function_create_native(c, watcher, 0, 0, &fn) == FVM_OK);
        fvm_value_string(args, "0"); args[1] = object(fn); assert(fvm_object_call(c, o, "watch", 2, args, &out) == FVM_OK && out.boolean);
        fvm_value_number(args, 0); fvm_value_number(args + 1, 0); fvm_value_number(args + 2, 81); watched = 0;
        assert(fvm_object_call(c, o, "splice", 3, args, &out) == FVM_OK && watched == 0); number(c, o, "0", 81);
        assert(fvm_gc_pin(c, o) == FVM_OK);
        FVM_Object *tools = 0, *proto = 0;
        assert(fvm_construct(c, "Array", 0, 0, &tools) == FVM_OK); assert(fvm_gc_pin(c, tools) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK); assert(fvm_gc_pin(c, proto) == FVM_OK);
        assert(fvm_object_define_accessor_rooted(c, proto, "1", getter, 0, 0, 0) == FVM_OK);
        fvm_value_number(&out, 3); assert(fvm_construct(c, "Array", 1, &out, &o) == FVM_OK);
        assert(fvm_object_set_prototype(c, o, proto) == FVM_OK);
        assert(fvm_object_set(c, o, "0", values) == FVM_OK); assert(fvm_object_set(c, o, "2", values + 2) == FVM_OK);
        fvm_value_number(args + 1, 1); reads = 0;
        assert(borrow(c, tools, "splice", o, 2, args, &out) == FVM_OK && reads == 0);
        int own = 0; assert(fvm_object_has_own(c, o, "0", &own) == FVM_OK && !own); number(c, o, "1", 7);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
