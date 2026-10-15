/* Loaded native and borrowed sorting crosses 65536 with bounded snapshots. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        for (unsigned variant = 0; variant < 4; ++variant) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
            unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
            FVM_Object *array = 0, *source = 0; FVM_Value value, args[2], out;
            assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
            assert(fvm_construct(c, variant & 1 ? "Object" : "Array", 0, 0, &source) == FVM_OK);
            fvm_value_number(&value, 65537); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
            fvm_value_number(&value, 19); assert(fvm_object_set(c, source, "0", &value) == FVM_OK);
            if (variant < 2) fvm_value_number(args, 16);
            else { fvm_value_string(args, "field"); fvm_value_number(args + 1, 16); }
            unsigned long old_limit = rt->config.instruction_limit; rt->config.instruction_limit = 8;
            assert(borrow(c, array, variant < 2 ? "sort" : "sortOn", source, variant < 2 ? 1 : 2, args, &out) == FVM_ERROR);
            rt->config.instruction_limit = old_limit;
            number(c, source, "length", 65537); number(c, source, "0", 19);
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
        }
    }
    for (unsigned variant = 0; variant < 4; ++variant) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        const unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *source = 0; FVM_Value value, args[2], out;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
        assert(fvm_construct(c, variant & 1 ? "Object" : "Array", 0, 0, &source) == FVM_OK);
        assert(fvm_gc_pin(c, source) == FVM_OK);
        fvm_value_number(&value, 65537); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        for (unsigned long i = 0; i < 65537; ++i) {
            fvm_value_number(&value, (i * 251UL) % 65537UL);
            assert(fvm_table_set_index(c, source, i, &value) == FVM_OK);
        }
        if (variant < 3) fvm_value_number(args, variant == 2 ? 24 : 16);
        else { fvm_value_string(args, "field"); fvm_value_number(args + 1, 16); }
        assert(borrow(c, array, variant == 3 ? "sortOn" : "sort", source, variant == 3 ? 2 : 1, args, &out) == FVM_OK);
        assert(out.type == FVM_VALUE_OBJECT); assert(fvm_gc_pin(c, out.object) == FVM_OK);
        if (variant == 2) {
            number(c, source, "1", 251);
            for (unsigned long i = 0; i < 65537; ++i) {
                assert(fvm_table_get_index(c, out.object, i, &value) == FVM_OK && value.type == FVM_VALUE_NUMBER);
                unsigned long index = (unsigned long)value.number;
                assert(index < 65537); assert(fvm_table_get_index(c, source, index, &value) == FVM_OK && value.number == i);
            }
        } else {
            assert(out.object == source);
            for (unsigned long i = 0; i < 65537; ++i) assert(fvm_table_get_index(c, source, i, &value) == FVM_OK && value.type == FVM_VALUE_NUMBER && value.number == i);
        }
        number(c, out.object, "length", 65537); assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(c->gc_frames == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    for (unsigned variant = 0; variant < 4; ++variant) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *source = 0; FVM_Value value, args[2], out;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
        assert(fvm_construct(c, variant & 1 ? "Object" : "Array", 0, 0, &source) == FVM_OK);
        fvm_value_number(&value, 65537); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        if (variant < 2) fvm_value_number(args, 16);
        else { fvm_value_string(args, "field"); fvm_value_number(args + 1, 16); }
        assert(borrow(c, array, variant < 2 ? "sort" : "sortOn", source, variant < 2 ? 1 : 2, args, &out) == (variant & 1 ? FVM_UNSUPPORTED : FVM_OUT_OF_MEMORY));
        fvm_context_destroy(c);
    }
    {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        const unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *source = 0; FVM_Value value, args[2], out;
        fvm_value_number(&value, 4294967295.0); assert(fvm_construct(c, "Array", 1, &value, &source) == FVM_OK);
        fvm_value_number(args, 16); assert(fvm_object_call(c, source, "sort", 1, args, &out) == FVM_UNSUPPORTED);
        fvm_value_string(args, "field"); fvm_value_number(args + 1, 16);
        assert(fvm_object_call(c, source, "sortOn", 2, args, &out) == FVM_UNSUPPORTED);
        assert(c->gc_frames == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
