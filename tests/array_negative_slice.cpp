/* Negative loaded slice selects signed own data after capturing length. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static FVM_Result start_value(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *data)
{
    FVM_Value value; fvm_value_number(&value, 99);
    assert(fvm_object_set(c, (FVM_Object *)data, "length", &value) == FVM_OK);
    fvm_value_number(out, 0); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *o = 0, *counter = 0; FVM_Value value, args[2], out;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
        assert(fvm_gc_pin(c, array) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
        fvm_value_number(&value, -3); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        fvm_value_number(&value, 11); assert(fvm_object_set(c, o, "-3", &value) == FVM_OK);
        fvm_value_number(&value, 12); assert(fvm_object_set(c, o, "-1", &value) == FVM_OK);
        fvm_value_number(args, 0); fvm_value_number(args + 1, -1);
        assert(borrow(c, array, "slice", o, 2, args, &out) == FVM_OK);
        number(c, out.object, "length", 3); number(c, out.object, "0", 11); number(c, out.object, "2", 12);
        int own = 0; assert(fvm_object_has_own(c, out.object, "1", &own) == FVM_OK && own);
        assert(fvm_object_get(c, out.object, "1", &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED);
        assert(borrow(c, array, "slice", o, 1, args, &out) == FVM_OK); number(c, out.object, "length", 0);
        fvm_value_undefined(args + 1);
        assert(borrow(c, array, "slice", o, 2, args, &out) == FVM_OK); number(c, out.object, "length", 0);
        fvm_value_undefined(args); fvm_value_number(args + 1, -1);
        assert(borrow(c, array, "slice", o, 2, args, &out) == FVM_OK); number(c, out.object, "length", 3);
        fvm_value_number(args, -1); fvm_value_number(args + 1, -1);
        assert(borrow(c, array, "slice", o, 2, args, &out) == FVM_OK); number(c, out.object, "length", 0);
        fvm_value_number(args, -2147483648.0);
        assert(borrow(c, array, "slice", o, 2, args, &out) == FVM_UNSUPPORTED);
        assert(fvm_object_define_accessor_rooted(c, o, "-2", getter, 0, 0, 0) == FVM_OK);
        fvm_value_number(args, 0); reads = 0;
        assert(borrow(c, array, "slice", o, 2, args, &out) == FVM_OK && reads == 0);
        assert(fvm_object_get(c, out.object, "1", &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED);
        assert(fvm_construct(c, "Object", 0, 0, &counter) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, counter, "valueOf", start_value, o, o) == FVM_OK);
        args[0] = object(counter);
        assert(borrow(c, array, "slice", o, 2, args, &out) == FVM_OK); number(c, out.object, "length", 3); number(c, o, "length", 99);
        fvm_value_number(&value, 4294967293.0); assert(fvm_object_set(c, array, "length", &value) == FVM_OK);
        fvm_value_string(&value, "kept"); assert(fvm_object_set(c, array, "-3", &value) == FVM_OK);
        /* Ordinary signed assignment now grows length; restore the captured slice fixture. */
        number(c, array, "length", -2);
        fvm_value_number(&value, 4294967293.0); assert(fvm_object_set(c, array, "length", &value) == FVM_OK);
        fvm_value_number(args, 0);
        assert(borrow(c, array, "slice", array, 2, args, &out) == FVM_OK); number(c, out.object, "length", 3);
        assert(fvm_gc_pin(c, out.object) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(fvm_object_get(c, out.object, "0", &value) == FVM_OK && value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "kept"));
        unsigned long old_limit = rt->config.instruction_limit; rt->config.instruction_limit = 1;
        assert(borrow(c, array, "slice", array, 2, args, &out) == FVM_ERROR);
        rt->config.instruction_limit = old_limit; number(c, array, "length", 4294967293.0);
        assert(c->gc_frames == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
