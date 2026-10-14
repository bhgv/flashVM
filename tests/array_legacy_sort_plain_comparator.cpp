#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
static FVM_Result fail_call(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{ assert(false); return FVM_ERROR; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *plain = 0, *fn = 0; assert(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK); assert(fvm_function_create_native(c, fail_call, 0, 0, &fn) == FVM_OK);
        FVM_Value value = object(fn); assert(fvm_object_set(c, plain, "call", &value) == FVM_OK); assert(fvm_object_set(c, plain, "valueOf", &value) == FVM_OK);
        FVM_Value values[3], args[2], out; fvm_value_number(values, 10); fvm_value_number(values+1, 2); fvm_value_number(values+2, 3);
        const unsigned flags[] = {0,2,8,10,4,12};
        for (unsigned mode = 0; mode < 6; ++mode) {
            FVM_Object *array = 0; assert(fvm_construct(c, "Array", 3, values, &array) == FVM_OK); args[0] = object(plain); fvm_value_number(args+1, flags[mode]);
            assert(fvm_object_call(c, array, "sort", 2, args, &out) == (version ? FVM_OK : FVM_UNSUPPORTED));
            if (version) {
                assert(out.type == FVM_VALUE_OBJECT);
                for (unsigned i = 0; i < 3; ++i) {
                    assert(fvm_table_get_index(c, out.object, i, &value) == FVM_OK);
                    unsigned source = flags[mode] & 2 ? 2-i : i;
                    assert(value.number == (flags[mode] & 8 ? source : values[source].number));
                    if (flags[mode] & 8) { assert(fvm_table_get_index(c, array, i, &value) == FVM_OK && value.number == values[i].number); }
                }
            }
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
        }
        if (version) {
            FVM_Object *array = 0; values[1] = values[0]; assert(fvm_construct(c, "Array", 3, values, &array) == FVM_OK);
            fvm_value_number(args+1, 4); assert(fvm_object_call(c, array, "sort", 2, args, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 0);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
