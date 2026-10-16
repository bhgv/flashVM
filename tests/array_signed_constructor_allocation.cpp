/* Signed Array construction and graph restoration leave no published partial result on OOM. */
#define main signed_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "flashvm/serialization.h"
#include <limits>
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const double values[] = {-2.9, 2.9, std::numeric_limits<double>::quiet_NaN()};
    const double expected[] = {-2, 2, -2147483648.0};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 3; ++kind) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
            unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
            FVM_Value input, length; fvm_value_number(&input, values[kind]);
            FVM_Object *array = (FVM_Object *)1;
            allowance = boundary;
            FVM_Result status = fvm_construct(c, "Array", 1, &input, &array);
            allowance = -1;
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true; assert(!array && fvm_gc_collect(c, 0) == FVM_OK);
                assert(fvm_construct(c, "Array", 1, &input, &array) == FVM_OK);
            } else { assert(status == FVM_OK); success = true; }
            assert(fvm_gc_pin(c, array) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_object_get(c, array, "length", &length) == FVM_OK && length.number == expected[kind]);
            assert(!c->gc_frames && !c->gc_native_depth && !c->exception_pending);
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    /* A restored negative array must not depend on installed destination builtins. */
    FVM_Context *source = 0; assert(fvm_context_create(rt, &source) == FVM_OK);
    unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
    assert(fvm_load_memory(source, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(source) == FVM_OK);
    FVM_Value value; fvm_value_number(&value, -3); FVM_Object *array = 0;
    assert(fvm_construct(source, "Array", 1, &value, &array) == FVM_OK);
    value.type = FVM_VALUE_OBJECT; value.object = array;
    assert(fvm_object_set(source, array, "cycle", &value) == FVM_OK && fvm_gc_pin(source, array) == FVM_OK);
    unsigned char *data = 0; size_t bytes = 0;
    assert(fvm_value_serialize(source, &value, &data, &bytes) == FVM_OK);
    bool failed = false, success = false;
    for (long boundary = 0; boundary < 4096 && !success; ++boundary) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned long before = 0, after = 0; assert(fvm_gc_object_count(c, &before) == FVM_OK);
        allowance = boundary; FVM_Result status = fvm_value_deserialize(c, data, bytes, &value); allowance = -1;
        if (status == FVM_OUT_OF_MEMORY) {
            failed = true;
            assert(value.type == FVM_VALUE_UNDEFINED && fvm_gc_object_count(c, &after) == FVM_OK && before == after);
            assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_value_deserialize(c, data, bytes, &value) == FVM_OK);
        } else { assert(status == FVM_OK); success = true; }
        FVM_Value length; assert(fvm_object_get(c, value.object, "length", &length) == FVM_OK && length.number == -3);
        assert(fvm_gc_pin(c, value.object) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
        assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    assert(failed && success);
    fvm_serialized_free(data); fvm_context_destroy(source); fvm_runtime_destroy(rt);
}
