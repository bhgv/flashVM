/* Loaded builtin sort converts operands in comparison order, not once per element. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
static const int expected[] = {3,1,3,4,3,2,3,2,3,4,3,2,2,1};
static unsigned converted, error_after;
static FVM_Result text(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    FVM_Value value; assert(fvm_object_get(c, self, "rank", &value) == FVM_OK);
    assert(converted < sizeof(expected) / sizeof(expected[0]) && value.number == expected[converted]);
    ++converted;
    if (error_after && converted == error_after) return FVM_ERROR;
    static const char *keys[] = {"1","2","3","4"}; return fvm_value_string(out, keys[(unsigned)value.number - 1]);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *fn = 0; assert(fvm_function_create_native(c, text, 0, 0, &fn) == FVM_OK);
        const int initial[] = {3,1,4,2}; FVM_Value values[4], value, out, flags;
        for (unsigned i = 0; i < 4; ++i) {
            FVM_Object *row = 0; assert(fvm_construct(c, "Object", 0, 0, &row) == FVM_OK);
            fvm_value_number(&value, initial[i]); assert(fvm_object_set(c, row, "rank", &value) == FVM_OK);
            value = object(fn); assert(fvm_object_set(c, row, "toString", &value) == FVM_OK); values[i] = object(row);
        }
        for (unsigned mode = 0; mode < 3; ++mode) {
            FVM_Object *array = 0; assert(fvm_construct(c, "Array", 4, values, &array) == FVM_OK);
            converted = 0; error_after = mode == 2 ? 2 : 0; fvm_value_number(&flags, mode == 1 ? 2 : 0);
            assert(fvm_object_call(c, array, "sort", 1, &flags, &out) == (mode == 2 ? FVM_ERROR : FVM_OK));
            assert(converted == (mode == 2 ? 2U : 14U));
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
            assert(fvm_table_get_index(c, array, 0, &value) == FVM_OK);
            assert(value.object == values[mode == 2 ? 0 : mode == 1 ? 2 : 1].object);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
