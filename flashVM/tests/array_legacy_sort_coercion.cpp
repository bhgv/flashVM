/* Loaded builtin sort converts operands in comparison order, not once per element. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
static const int expected[] = {3,1,3,4,3,2,3,2,3,4,3,2,2,1};
static const int descending[] = {3,1,3,2,3,4,3,4,3,1,3,4,1,2};
static const int changed[] = {3,1,4,2,4,1,1,4,1,2,1,4,4,2};
static const int *sequence = expected;
static unsigned converted, error_after, getter_calls;
static FVM_Object *changed_record;
static bool change_field;
static const char field[] = {'k',0,'y'};
static FVM_Result getter_spy(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++getter_calls; fvm_value_number(out, 100); return FVM_OK; }
static FVM_Result text(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    FVM_Value value; assert(fvm_object_get(c, self, "rank", &value) == FVM_OK);
    assert(converted < sizeof(expected) / sizeof(expected[0]) && value.number == sequence[converted]);
    ++converted;
    if (change_field && converted == 1) {
        FVM_Value zero; fvm_value_number(&zero, 0);
        assert(fvm_object_set_bytes(c, changed_record, field, sizeof(field), &zero) == FVM_OK);
    }
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
        FVM_Object *records[4], *get_fn = 0; FVM_Value args[3];
        assert(fvm_function_create_native(c, getter_spy, 0, 0, &get_fn) == FVM_OK);
        for (unsigned i = 0; i < 4; ++i) {
            assert(fvm_construct(c, "Object", 0, 0, records + i) == FVM_OK);
            assert(fvm_object_set_bytes(c, records[i], field, sizeof(field), values + i) == FVM_OK);
            fvm_value_string_bytes(args, field, sizeof(field)); args[1] = object(get_fn); fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL;
            assert(fvm_object_call(c, records[i], "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        }
        for (unsigned mode = 0; mode < 4; ++mode) {
            FVM_Value input[4];
            for (unsigned i = 0; i < 4; ++i) { assert(fvm_object_set_bytes(c, records[i], field, sizeof(field), values + i) == FVM_OK); input[i] = object(records[i]); }
            FVM_Object *array = 0; assert(fvm_construct(c, "Array", 4, input, &array) == FVM_OK);
            fvm_value_string_bytes(args, field, sizeof(field)); fvm_value_number(args + 1, mode == 1 ? 2 : 0);
            converted = getter_calls = 0; error_after = mode == 3 ? 2 : 0;
            change_field = mode == 2; changed_record = records[0]; sequence = change_field ? changed : mode == 1 ? descending : expected;
            assert(fvm_object_call(c, array, "sortOn", 2, args, &out) == (mode == 3 ? FVM_ERROR : FVM_OK));
            assert(converted == (mode == 3 ? 2U : 14U) && getter_calls == 0);
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
            assert(fvm_table_get_index(c, array, 0, &value) == FVM_OK);
            assert(value.object == records[mode >= 2 ? 0 : mode == 1 ? 2 : 1]);
            if (mode == 2) { assert(fvm_table_get_index(c, array, 3, &value) == FVM_OK && value.object == records[2]); }
            change_field = false; sequence = expected;
        }
        FVM_Object *proto = 0, *plain = 0, *array = 0;
        assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
        assert(fvm_object_define_accessor_rooted_bytes(c, proto, field, sizeof(field), getter_spy, 0, 0, 0) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK); assert(fvm_object_set_prototype(c, plain, proto) == FVM_OK);
        fvm_value_number(&value, 1); assert(fvm_object_set_bytes(c, records[0], field, sizeof(field), &value) == FVM_OK);
        FVM_Value pair[2]; pair[0] = object(records[0]); pair[1] = object(plain);
        assert(fvm_construct(c, "Array", 2, pair, &array) == FVM_OK);
        fvm_value_string_bytes(args, field, sizeof(field)); fvm_value_number(args + 1, 16); getter_calls = 0; error_after = 0;
        assert(fvm_object_call(c, array, "sortOn", 2, args, &out) == FVM_OK && getter_calls == 0);
        assert(fvm_table_get_index(c, array, 0, &value) == FVM_OK && value.object == (version < 7 ? plain : records[0]));
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
