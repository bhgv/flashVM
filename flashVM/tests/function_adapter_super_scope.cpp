/* Adapter superclass depth and rooted invocation identities restore through nesting and failures. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include "function_adapter_super.cpp"
#include "../src/core/call_scope.hpp"
#include "flashvm/exception.h"
static unsigned adapter_scope_mode;
static FVM_Object *adapter_scope_function, *adapter_scope_parent, *adapter_scope_inner, *adapter_scope_inner_this;
static FVM_Result adapter_scope_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    assert(c->calling_function == adapter_scope_function && c->calling_owner == adapter_scope_parent && c->calling_super_depth == 1);
    assert(frame->argc == 1 && frame->argv[0].number == 81);
    if (adapter_scope_mode == 2) {
        FVM_Value value = class_value(adapter_scope_inner_this), nested;
        assert(fvm_object_call(c, adapter_scope_inner, "call", 1, &value, &nested) == FVM_OK && nested.type == FVM_VALUE_OBJECT && nested.object->resource_kind == FVM_RESOURCE_SUPER);
        assert(nested.object->resource_result.object->properties["depth"].number == 1 && fvm_gc_pin(c, nested.object) == FVM_OK);
        assert(c->calling_function == adapter_scope_function && c->calling_owner == adapter_scope_parent && c->calling_super_depth == 1);
    }
    if (adapter_scope_mode == 3) assert(fvm_object_set_prototype(c, frame->receiver, 0) == FVM_OK);
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    bool alive = false; for (size_t i = 0; i < c->objects.size(); ++i) if (c->objects[i] == adapter_scope_parent) alive = true; assert(alive);
    fvm_value_number(out, 81); if (adapter_scope_mode == 1) return fvm_exception_raise(c, out);
    return adapter_scope_mode == 4 ? FVM_ERROR : FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned apply = 0; apply < 2; ++apply)
    for (adapter_scope_mode = 0; adapter_scope_mode < 5; ++adapter_scope_mode) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        assert(c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH);
        FVM_Object *receiver = 0, *capture = 0, *outer = 0, *outer_owner = 0, *array = 0;
        assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &adapter_scope_parent) == FVM_OK && fvm_construct(c, "Object", 0, 0, &capture) == FVM_OK);
        assert(fvm_object_set_prototype(c, receiver, adapter_scope_parent) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &adapter_scope_inner_this) == FVM_OK && fvm_object_set_prototype(c, adapter_scope_inner_this, 0) == FVM_OK);
        adapter_scope_inner = adapter_super_function(c, 0); FVM_Value value = class_value(adapter_scope_inner), args[2], out;
        assert(fvm_object_set(c, capture, "inner", &value) == FVM_OK); value = class_value(adapter_scope_inner_this); assert(fvm_object_set(c, capture, "innerThis", &value) == FVM_OK);
        adapter_scope_function = adapter_super_function(c, 0); assert(fvm_gc_pin(c, adapter_scope_function) == FVM_OK && fvm_function_set_body(c, adapter_scope_function, adapter_scope_body, 0, capture) == FVM_OK);
        outer = adapter_super_function(c, 0); assert(fvm_construct(c, "Object", 0, 0, &outer_owner) == FVM_OK);
        args[0] = class_value(receiver); fvm_value_number(args + 1, 81);
        if (apply) { assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_table_set_index(c, array, 0, args + 1) == FVM_OK); args[1] = class_value(array); }
        {
            FVM_CallScope outer_scope(c, 0, outer, outer_owner, 7);
            const FVM_Result expected = adapter_scope_mode == 1 ? FVM_EXCEPTION : adapter_scope_mode == 4 ? FVM_ERROR : FVM_OK;
            assert(fvm_object_call(c, adapter_scope_function, apply ? "apply" : "call", 2, args, &out) == expected);
            assert(c->calling_super_depth == 7 && c->calling_function == outer && c->calling_owner == outer_owner);
            if (expected == FVM_EXCEPTION) { assert(fvm_exception_get(c, &out) == FVM_OK && out.number == 81); assert(fvm_exception_clear(c) == FVM_OK); }
        }
        assert(c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH && !c->calling_function && !c->calling_owner && !c->gc_frames && !c->gc_native_depth);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
