/* Loaded sorting reverses DESCENDING results after comparison, including equal keys. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static unsigned compared;
static bool equal_keys;
static FVM_Result compare(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{ assert(n == 2); ++compared; fvm_value_number(out, equal_keys ? 0 : a[0].number - a[1].number); return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value values[3], args[2], out; FVM_Object *array = 0, *fn = 0;
        fvm_value_number(values, 2); fvm_value_number(values + 1, 1); fvm_value_number(values + 2, 3);
        assert(fvm_construct(c, "Array", 3, values, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_function_create_native(c, compare, 0, 0, &fn) == FVM_OK); assert(fvm_gc_pin(c, fn) == FVM_OK);
        args[0] = object(fn); fvm_value_number(args + 1, 2); compared = 0;
        assert(fvm_object_call(c, array, "sort", 2, args, &out) == FVM_OK && compared);
        number(c, array, "0", 3); number(c, array, "2", 1);
        FVM_Object *o = 0; assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
        FVM_Value value; fvm_value_number(&value, 3); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        for (unsigned i = 0; i < 3; ++i) assert(fvm_table_set_index(c, o, i, values + i) == FVM_OK);
        FVM_Object *proto = 0; assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
        assert(fvm_object_define_accessor_rooted(c, proto, "1", getter, 0, 0, 0) == FVM_OK);
        assert(fvm_object_set_prototype(c, o, proto) == FVM_OK);
        fvm_value_number(args + 1, 10); compared = reads = 0;
        assert(borrow(c, array, "sort", o, 2, args, &out) == FVM_OK && compared && reads == 0 && out.type == FVM_VALUE_OBJECT && out.object != o);
        number(c, out.object, "0", 2); number(c, out.object, "1", 0); number(c, out.object, "2", 1);
        number(c, o, "0", 2); number(c, o, "1", 1); number(c, o, "2", 3);
        equal_keys = true;
        assert(borrow(c, array, "sort", o, 2, args, &out) == FVM_OK);
        number(c, out.object, "0", version ? 2 : 0);
        number(c, out.object, "1", 1);
        number(c, out.object, "2", version ? 0 : 2);
        equal_keys = false;
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
