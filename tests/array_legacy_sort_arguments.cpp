#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include <cmath>
static FVM_Result comparator(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    assert(n == 2); fvm_value_number(out, a[0].number - a[1].number); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const double options[] = {16.9, 48, 4294967312.0, -4294967280.0, HUGE_VAL, -HUGE_VAL, HUGE_VAL-HUGE_VAL, 0};
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value values[2], args[2], out, value; fvm_value_number(values, 10); fvm_value_number(values+1, 2);
        for (unsigned mode = 0; mode < 8; ++mode) {
            FVM_Object *array = 0; assert(fvm_construct(c, "Array", 2, values, &array) == FVM_OK);
            fvm_value_number(args, options[mode]); assert(fvm_object_call(c, array, "sort", 1, args, &out) == FVM_OK && out.object == array);
            assert(fvm_table_get_index(c, array, 0, &value) == FVM_OK && value.number == (mode < 4 ? 2 : 10));
        }
        for (unsigned mode = 0; mode < 4; ++mode) {
            FVM_Object *array = 0; assert(fvm_construct(c, "Array", 2, values, &array) == FVM_OK);
            if (!mode) fvm_value_string(args, "16"); else if (mode == 1) fvm_value_boolean(args, 1); else if (mode == 2) { fvm_value_undefined(args); args[0].type = FVM_VALUE_NULL; } else fvm_value_undefined(args);
            assert(fvm_object_call(c, array, "sort", 1, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
            assert(fvm_table_get_index(c, array, 0, &value) == FVM_OK && value.number == 10);
            assert(fvm_object_call(c, array, "sortOn", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        }
        FVM_Object *array = 0; assert(fvm_construct(c, "Array", 2, values, &array) == FVM_OK);
        fvm_value_number(args, 0); fvm_value_number(args+1, 16); assert(fvm_object_call(c, array, "sort", 2, args, &out) == FVM_OK);
        assert(fvm_table_get_index(c, array, 0, &value) == FVM_OK && value.number == 2);
        FVM_Object *fn = 0; assert(fvm_function_create_native(c, comparator, 0, 0, &fn) == FVM_OK);
        assert(fvm_construct(c, "Array", 2, values, &array) == FVM_OK);
        args[0] = object(fn); fvm_value_string(args+1, "2"); assert(fvm_object_call(c, array, "sort", 2, args, &out) == FVM_OK);
        assert(fvm_table_get_index(c, array, 0, &value) == FVM_OK && value.number == 2);
        assert(c->gc_frames == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
