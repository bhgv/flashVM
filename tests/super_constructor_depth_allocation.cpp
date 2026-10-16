/* Explicit-super constructor depth survives allocation failure and permits retry. */
#define main super_constructor_depth_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include "function_adapter_super.cpp"
#include "../src/core/call_scope.hpp"
#include "../src/standard/super.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 1024 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
            FVM_Object *receiver = 0, *parent = 0; assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
            assert(fvm_object_set_property_flags(c, receiver, "__constructor__", 0, 4) == FVM_OK);
            FVM_Value value = class_value(adapter_super_function(c, 0)), out; assert(fvm_object_set(c, receiver, "__constructor__", &value) == FVM_OK);
            assert(fvm_object_set_prototype(c, receiver, 0) == FVM_OK);
            FVM_Value facade; { FVM_CallScope scope(c, 0, value.object, 0, 0); assert(fvm_super_create(c, value.object, receiver, &facade) == FVM_OK); }
            FVM_Object *target = facade.object; assert(fvm_gc_pin(c, target) == FVM_OK);
            fvm_value_string_bytes(&value, "r\0s", 3); allowance = boundary;
            FVM_Result status = fvm_function_invoke(c, target, receiver, 1, &value, &out); allowance = -1;
            assert(!c->resolve_depth && !c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_owner && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH);
            if (status == FVM_OUT_OF_MEMORY) { failed = true; assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_function_invoke(c, target, receiver, 1, &value, &out) == FVM_OK); }
            else { assert(status == FVM_OK); success = true; }
            assert(out.type == FVM_VALUE_OBJECT && out.object->resource_result.object->properties["depth"].number == 1);
            assert(fvm_gc_pin(c, out.object) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK); fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
