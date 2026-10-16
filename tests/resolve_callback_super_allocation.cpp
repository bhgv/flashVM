/* Fixed resolver depth restores invocation state after allocation failure and retry. */
#define main resolve_depth_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include "function_adapter_super.cpp"
#include "../src/core/call_scope_constants.h"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char key[] = "n\0m";
    for (unsigned version = 4; version <= 8; ++version) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 1024 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
            FVM_Object *receiver = 0; assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
            FVM_Value args[3], out; fvm_value_string_bytes(args, key, sizeof(key) - 1); args[1] = class_value(adapter_super_function(c, 0)); fvm_value_string_bytes(args + 2, "u\0d", 3);
            assert(fvm_object_set(c, receiver, "__resolve", args + 1) == FVM_OK && fvm_object_set_prototype(c, receiver, 0) == FVM_OK);
            FVM_Value value; fvm_value_string_bytes(&value, "r\0s", 3); allowance = boundary;
            FVM_Result status = fvm_object_get_bytes(c, receiver, key, sizeof(key) - 1, &out); allowance = -1;
            assert(!c->resolve_depth && !c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_owner && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH);
            if (status == FVM_OUT_OF_MEMORY) { failed = true; assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get_bytes(c, receiver, key, sizeof(key) - 1, &out) == FVM_OK); }
            else { assert(status == FVM_OK); success = true; }
            assert(fvm_object_get_bytes(c, receiver, key, sizeof(key) - 1, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT && out.object->resource_result.object->properties["depth"].number == 0);
            assert(fvm_gc_collect(c, 0) == FVM_OK); fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
