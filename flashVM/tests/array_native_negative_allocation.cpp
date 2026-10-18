/* Every new/new[] boundary in native signed end mutations restores call/root state. */
#define main native_signed_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *methods[] = {"push", "unshift", "pop", "shift"};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned method = 0; method < 4; ++method) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
            FVM_Value value, args[2], out; fvm_value_number(&value, -3); FVM_Object *array = 0;
            assert(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
            fvm_value_number(&value, 72); assert(fvm_base_store_data(c, array, "-4", value, false) == FVM_OK);
            fvm_value_number(&value, 81); assert(fvm_array_store_own(c, array, 0, value) == FVM_OK);
            assert(fvm_value_string_bytes(args, "first\0tail", 10) == FVM_OK);
            assert(fvm_value_string_bytes(args + 1, "second\0tail", 11) == FVM_OK);
            const unsigned argc = method < 2 ? 2 : 0;
            allowance = boundary; FVM_Result status = fvm_object_call(c, array, methods[method], argc, args, &out); allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->exception_pending);
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true;
                assert(fvm_gc_collect(c, 0) == FVM_OK);
                /* Mutations intentionally permit completed earlier stores to survive an error. */
                assert(fvm_object_call(c, array, methods[method], argc, args, &out) == FVM_OK);
            } else {
                assert(status == FVM_OK); success = true;
                FVM_Value length; assert(fvm_object_get(c, array, "length", &length) == FVM_OK && length.number == (method < 2 ? -1 : -4));
                if (!method) {
                    assert(fvm_object_get(c, array, "-3", &out) == FVM_OK && out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 10 && !std::memcmp(out.string, "first\0tail", 10));
                    assert(fvm_object_get(c, array, "-2", &out) == FVM_OK && out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 11 && !std::memcmp(out.string, "second\0tail", 11));
                }
            }
            assert(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth);
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
