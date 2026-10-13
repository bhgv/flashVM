/* Loaded splice removed arrays materialize own undefined without getter/prototype reads. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static void missing(FVM_Context *c, FVM_Object *o, unsigned long index)
{
    FVM_Value v; char key[32]; std::sprintf(key, "%lu", index); int own = 0;
    assert(fvm_table_get_index(c, o, index, &v) == FVM_OK && v.type == FVM_VALUE_UNDEFINED);
    assert(fvm_object_has_own_bytes(c, o, key, std::strlen(key), &own) == FVM_OK && own);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK);
    cfg.instruction_limit = 64; assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *o = 0, *proto = 0; FVM_Value v, args[2], out;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
        assert(fvm_gc_pin(c, array) == FVM_OK);
        for (unsigned generic = 0; generic < 2; ++generic) {
            fvm_value_number(&v, 4); assert(fvm_construct(c, "Array", 1, &v, &o) == FVM_OK);
            assert(fvm_gc_pin(c, o) == FVM_OK);
            fvm_value_string(&v, "B"); assert(fvm_object_set(c, o, "1", &v) == FVM_OK);
            if (generic) {
                assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
                assert(fvm_object_define_accessor_rooted(c, proto, "0", getter, 0, 0, 0) == FVM_OK);
                assert(fvm_object_set_prototype(c, o, proto) == FVM_OK);
                assert(fvm_object_define_accessor_rooted(c, o, "2", getter, 0, 0, 0) == FVM_OK);
            }
            fvm_value_number(args, 0); fvm_value_number(args + 1, 3); reads = 0;
            assert(borrow(c, array, "splice", o, 2, args, &out) == FVM_OK && reads == 0);
            assert(fvm_gc_pin(c, out.object) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
            number(c, out.object, "length", 3); missing(c, out.object, 0); missing(c, out.object, 2);
            assert(fvm_table_get_index(c, out.object, 1, &v) == FVM_OK && v.type == FVM_VALUE_STRING && !std::strcmp(v.string, "B"));
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
        }
        fvm_value_number(&v, 100); assert(fvm_construct(c, "Array", 1, &v, &o) == FVM_OK);
        fvm_value_number(args, 0); fvm_value_number(args + 1, 100);
        assert(fvm_object_call(c, o, "splice", 2, args, &out) == FVM_ERROR);
        number(c, o, "length", 100); assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
