/* Native signed splice allocation failures restore roots and permit retry of partial mutation. */
#define main native_splice_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "property_version_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
            FVM_Value value, args[4], out; fvm_value_number(&value, -3); FVM_Object *array = 0;
            assert(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
            fvm_value_number(args, 0); fvm_value_number(args + 1, 0);
            assert(fvm_value_string_bytes(args + 2, "first\0tail", 10) == FVM_OK);
            assert(fvm_value_string_bytes(args + 3, "second\0tail", 11) == FVM_OK);
            allowance = boundary; FVM_Result status = fvm_object_call(c, array, "splice", 4, args, &out); allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->exception_pending);
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true; assert(fvm_gc_collect(c, 0) == FVM_OK);
                assert(fvm_object_call(c, array, "splice", 4, args, &out) == FVM_OK);
            } else {
                assert(status == FVM_OK); success = true;
                assert(out.type == FVM_VALUE_OBJECT && fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == 0);
                assert(fvm_object_get(c, array, "length", &value) == FVM_OK && value.number == -1);
                assert(fvm_object_get(c, array, "-3", &value) == FVM_OK && value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 10 && !std::memcmp(value.string, "first\0tail", 10));
                assert(fvm_object_get(c, array, "-2", &value) == FVM_OK && value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 11 && !std::memcmp(value.string, "second\0tail", 11));
            }
            assert(fvm_gc_collect(c, 0) == FVM_OK); fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
