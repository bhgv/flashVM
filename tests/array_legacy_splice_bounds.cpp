/* Loaded splice wraps start/delete counts to Int32 without repeating conversion. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
static FVM_Object *source(FVM_Context *c)
{
    FVM_Value values[4]; const char *items[] = {"A", "B", "C", "D"};
    for (unsigned i = 0; i < 4; ++i) fvm_value_string(values + i, items[i]);
    FVM_Object *o = 0; assert(fvm_construct(c, "Array", 4, values, &o) == FVM_OK); return o;
}
static unsigned conversions;
static FVM_Result index(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++conversions; fvm_value_number(out, 4294967296.0); return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value args[2], out, value; FVM_Object *o = source(c);
        fvm_value_number(args, 4294967296.0); fvm_value_string(args + 1, "4294967297");
        assert(fvm_object_call(c, o, "splice", 2, args, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
        number(c, out.object, "length", 1); number(c, o, "length", 3);
        assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK && !std::strcmp(value.string, "A"));
        assert(fvm_table_get_index(c, o, 0, &value) == FVM_OK && !std::strcmp(value.string, "B"));
        o = source(c); fvm_value_number(args, 4294967295.0); fvm_value_number(args + 1, 1);
        assert(fvm_object_call(c, o, "splice", 2, args, &out) == FVM_OK); number(c, o, "length", 3);
        assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK && !std::strcmp(value.string, "D"));
        o = source(c); fvm_value_number(args, 0); fvm_value_number(args + 1, 4294967295.0);
        assert(fvm_object_call(c, o, "splice", 2, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED); number(c, o, "length", 4);
        FVM_Object *endpoint = 0; assert(fvm_construct(c, "Object", 0, 0, &endpoint) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, endpoint, "valueOf", index, 0, 0) == FVM_OK);
        args[0] = object(endpoint); fvm_value_number(args + 1, 1); conversions = 0;
        assert(fvm_object_call(c, o, "splice", 2, args, &out) == FVM_OK && conversions == 1); number(c, o, "length", 3);
        assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK && !std::strcmp(value.string, "A"));
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
