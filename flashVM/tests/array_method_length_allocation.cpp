/* Cold native length wrappers propagate OOM without publishing a reader result. */
#define main method_length_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_METHOD_LENGTH_FIXTURE_ONLY
#include "array_method_length.cpp"
#include "../src/standard/array_generic.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned operation = 0; operation < 4; ++operation) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
            FVM_Object *array = method_length_array(c, 0, false); assert(fvm_gc_pin(c, array) == FVM_OK); FVM_Value value, out; fvm_value_number(&value, 81); long length = 77;
            allowance = boundary;
            FVM_Result status = !operation ? fvm_array_legacy_length(c, array, &length) : operation == 3 ? fvm_object_set(c, array, "3", &value) : fvm_object_call(c, array, operation == 1 ? "join" : "slice", 0, 0, &out);
            allowance = -1;
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true; if (!operation) assert(length == 77);
                assert(fvm_gc_collect(c, 0) == FVM_OK);
                status = !operation ? fvm_array_legacy_length(c, array, &length) : operation == 3 ? fvm_object_set(c, array, "3", &value) : fvm_object_call(c, array, operation == 1 ? "join" : "slice", 0, 0, &out);
                assert(status == FVM_OK);
            } else { assert(status == FVM_OK); success = true; }
            if (!operation) assert(length == 0);
            if (operation == 1) assert(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, ""));
            if (operation == 2) assert(out.type == FVM_VALUE_OBJECT && fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == 0);
            if (operation == 3) assert(fvm_object_get(c, array, "length", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 4);
            assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending); fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
