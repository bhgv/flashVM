/* Loaded slice uses own data and signed endpoints, retaining explicit undefined slots. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
static FVM_Result mutate(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    FVM_Object *source = (FVM_Object *)raw; FVM_Value value;
    fvm_value_number(&value, 1); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
    fvm_value_string(&value, "mutated"); assert(fvm_object_set(c, source, "0", &value) == FVM_OK);
    fvm_value_number(out, 0); return FVM_OK;
}
static void own_undefined(FVM_Context *c, FVM_Object *o, unsigned long index)
{
    char name[32]; std::sprintf(name, "%lu", index); FVM_Value value; int own = 0;
    assert(fvm_table_get_index(c, o, index, &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED);
    assert(fvm_object_has_own_bytes(c, o, name, std::strlen(name), &own) == FVM_OK && own);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK);
    cfg.instruction_limit = 64; assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *proto = 0, *source = 0; FVM_Value value, result, args[2];
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
        fvm_value_string(&value, "inherited"); assert(fvm_object_set(c, proto, "3", &value) == FVM_OK);
        for (unsigned real = 0; real < 2; ++real) {
            fvm_value_number(&value, 4);
            assert(fvm_construct(c, real ? "Array" : "Object", real ? 1 : 0, real ? &value : 0, &source) == FVM_OK);
            assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
            assert(fvm_object_set_prototype(c, source, proto) == FVM_OK);
            fvm_value_string(&value, "A"); assert(fvm_object_set(c, source, "0", &value) == FVM_OK);
            assert(fvm_object_define_accessor_rooted(c, source, "1", getter, 0, 0, 0) == FVM_OK);
            reads = 0; assert(borrow(c, array, "slice", source, 0, 0, &result) == FVM_OK && reads == 0);
            number(c, result.object, "length", 4); for (unsigned i = 1; i < 4; ++i) own_undefined(c, result.object, i);
            fvm_value_number(args, 4294967296.0); fvm_value_number(args + 1, 4294967297.0);
            assert(borrow(c, array, "slice", source, 2, args, &result) == FVM_OK); number(c, result.object, "length", 1);
            assert(fvm_table_get_index(c, result.object, 0, &value) == FVM_OK && value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "A"));
            fvm_value_number(args, -1); fvm_value_undefined(args + 1);
            assert(borrow(c, array, "slice", source, 2, args, &result) == FVM_OK); number(c, result.object, "length", 1); own_undefined(c, result.object, 0);
        }
        assert(fvm_construct(c, "Object", 0, 0, &source) == FVM_OK);
        assert(fvm_object_define_accessor_rooted(c, source, "length", getter, 0, 0, 0) == FVM_OK);
        reads = 0; assert(borrow(c, array, "slice", source, 0, 0, &result) == FVM_OK && reads == 0); number(c, result.object, "length", 0);
        assert(fvm_construct(c, "Object", 0, 0, &source) == FVM_OK);
        fvm_value_number(&value, 4294967295.0); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        assert(borrow(c, array, "slice", source, 0, 0, &result) == FVM_OK); number(c, result.object, "length", 0);
        fvm_value_number(&value, 2147483647.0); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        assert(borrow(c, array, "slice", source, 0, 0, &result) == FVM_ERROR && c->avm1_execution_depth == 0);
        fvm_value_number(&value, 4); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        fvm_value_string(&value, "B"); assert(fvm_object_set(c, source, "2", &value) == FVM_OK);
        FVM_Object *endpoint = 0; assert(fvm_construct(c, "Object", 0, 0, &endpoint) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, endpoint, "valueOf", mutate, source, source) == FVM_OK);
        args[0] = object(endpoint);
        assert(borrow(c, array, "slice", source, 1, args, &result) == FVM_OK); number(c, result.object, "length", 4);
        assert(fvm_table_get_index(c, result.object, 0, &value) == FVM_OK && !std::strcmp(value.string, "mutated"));
        assert(fvm_table_get_index(c, result.object, 2, &value) == FVM_OK && !std::strcmp(value.string, "B"));
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
