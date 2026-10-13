/* Loaded concat copies own data, explicit missing slots and signed per-source lengths. */
#define main legacy_join_fixture_main
#include "array_legacy_join.cpp"
#undef main
static void undefined_slot(FVM_Context *c, FVM_Object *o, unsigned long index)
{
    FVM_Value value; char name[32]; std::sprintf(name, "%lu", index); int has = 0;
    assert(fvm_table_get_index(c, o, index, &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED);
    assert(fvm_object_has_own_bytes(c, o, name, std::strlen(name), &has) == FVM_OK && has);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK);
    cfg.instruction_limit = 64; assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *proto = 0, *source = 0, *argument = 0, *plain = 0;
        FVM_Value value, result, args[2]; assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
        fvm_value_string(&value, "inherited"); assert(fvm_object_set(c, proto, "3", &value) == FVM_OK);
        fvm_value_number(&value, 3); assert(fvm_construct(c, "Array", 1, &value, &argument) == FVM_OK);
        assert(fvm_object_define_accessor_rooted(c, argument, "0", getter, 0, 0, 0) == FVM_OK);
        fvm_value_string(&value, "B"); assert(fvm_object_set(c, argument, "1", &value) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK);
        fvm_value_number(&value, 99); assert(fvm_object_set(c, plain, "length", &value) == FVM_OK);
        args[0] = object(argument); args[1] = object(plain);
        for (unsigned real = 0; real < 2; ++real) {
            fvm_value_number(&value, 4); assert(fvm_construct(c, real ? "Array" : "Object", real ? 1 : 0, real ? &value : 0, &source) == FVM_OK);
            assert(fvm_object_set(c, source, "length", &value) == FVM_OK); assert(fvm_object_set_prototype(c, source, proto) == FVM_OK);
            fvm_value_string(&value, "A"); assert(fvm_object_set(c, source, "0", &value) == FVM_OK);
            assert(fvm_object_define_accessor_rooted(c, source, "1", getter, 0, 0, 0) == FVM_OK);
            reads = 0; assert(borrow(c, array, "concat", source, 2, args, &result) == FVM_OK && reads == 0);
            number(c, result.object, "length", 8);
            assert(fvm_table_get_index(c, result.object, 0, &value) == FVM_OK && !std::strcmp(value.string, "A"));
            for (unsigned i = 1; i < 5; ++i) undefined_slot(c, result.object, i);
            assert(fvm_table_get_index(c, result.object, 5, &value) == FVM_OK && !std::strcmp(value.string, "B"));
            undefined_slot(c, result.object, 6);
            assert(fvm_table_get_index(c, result.object, 7, &value) == FVM_OK && value.type == FVM_VALUE_OBJECT && value.object == plain);
        }
        assert(fvm_construct(c, "Object", 0, 0, &source) == FVM_OK);
        assert(fvm_object_define_accessor_rooted(c, source, "length", getter, 0, 0, 0) == FVM_OK);
        reads = 0; assert(borrow(c, array, "concat", source, 1, args, &result) == FVM_OK && reads == 0); number(c, result.object, "length", 3);
        fvm_value_number(&value, 4294967295.0); assert(fvm_object_set(c, argument, "length", &value) == FVM_OK);
        assert(borrow(c, array, "concat", source, 2, args, &result) == FVM_OK); number(c, result.object, "length", 1);
        assert(fvm_table_get_index(c, result.object, 0, &value) == FVM_OK && value.object == plain);
        fvm_value_number(&value, 2147483647.0); assert(fvm_object_set(c, argument, "length", &value) == FVM_OK);
        assert(borrow(c, array, "concat", source, 1, args, &result) == FVM_ERROR && c->avm1_execution_depth == 0);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
