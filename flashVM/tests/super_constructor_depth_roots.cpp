/* Explicit-super constructor depth survives getter deletion, missing parents and collection. */
#define FVM_PROPERTY_CALLBACK_SUPER_FIXTURE_ONLY
#include "property_callback_super.cpp"
#include "../src/core/call_scope.hpp"
#include "../src/standard/super.hpp"
#include "flashvm/exception.h"
static FVM_Object *receiver, *owner, *method, *capture, *selected_parent;
static unsigned expected_depth, mode;
static FVM_Result getter_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    assert(frame->receiver == owner);
    int removed = 0; assert(fvm_object_delete(c, owner, "__constructor__", &removed) == FVM_OK && removed);
    assert(fvm_object_set_prototype(c, receiver, 0) == FVM_OK && fvm_object_set_prototype(c, owner, 0) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
    *out = class_value(frame->capture); return FVM_OK;
}
static FVM_Result method_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    assert(frame->receiver == receiver && c->calling_owner == selected_parent && c->calling_constructor == method && c->calling_function == method && c->calling_super_depth == expected_depth);
    assert(fvm_gc_collect(c, 0) == FVM_OK); bool owner_live = false, capture_live = false, parent_live = false;
    for (size_t i = 0; i < c->objects.size(); ++i) { parent_live |= c->objects[i] == selected_parent; owner_live |= c->objects[i] == owner; capture_live |= c->objects[i] == capture; } assert(owner_live && capture_live && parent_live);
    if (mode == 1) { fvm_value_number(out, 77); return fvm_exception_raise(c, out); }
    if (mode == 2) return FVM_ERROR;
    return fvm_super_create(c, method, receiver, out);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned base = 0; base < 3; ++base)
        for (mode = 0; mode < 3; ++mode)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *nodes[7]; for (unsigned i = 0; i < 7; ++i) assert(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK);
        receiver = nodes[0]; owner = nodes[base]; expected_depth = base + 1;
        assert(fvm_gc_pin(c, receiver) == FVM_OK && fvm_construct(c, "Object", 0, 0, &capture) == FVM_OK);
        method = adapter_super_function(c, 0); assert(fvm_function_set_body(c, method, method_body, 0, capture) == FVM_OK);
        FVM_Object *getter = adapter_super_function(c, 0); assert(fvm_function_set_body(c, getter, getter_body, 0, method) == FVM_OK);
        assert(fvm_object_set_property_flags(c, owner, "__constructor__", 0, 4) == FVM_OK);
        callback_super_property(c, owner, "__constructor__", 15, getter, version_function(c, 0));
        for (unsigned i = 0; i < 7; ++i) assert(fvm_object_set_prototype(c, nodes[i], i < 6 ? nodes[i + 1] : 0) == FVM_OK);
        selected_parent = owner->prototype; FVM_Value value, out; { FVM_CallScope scope(c, 0, method, 0, base); assert(fvm_super_create(c, method, receiver, &value) == FVM_OK); } FVM_Object *target = value.object; assert(fvm_set_global(c, "capturedMemberTarget", &value) == FVM_OK);
        FVM_Result expected = mode == 1 ? FVM_EXCEPTION : mode == 2 ? FVM_ERROR : FVM_OK;
        { FVM_CallScope outer(c, 0, 0, 0, 7);
          if (!route) { assert(fvm_function_invoke(c, target, receiver, 0, 0, &out) == expected); if (mode == 1) { assert(fvm_exception_get(c, &out) == FVM_OK && out.number == 77); assert(fvm_exception_clear(c) == FVM_OK); } }
          else { Program code; code.number(0); code.variable("capturedMemberTarget"); code.text(""); code.op(0x52); code.finish(); out = boxing_run(c, code, route - 1, expected); }
          assert(c->calling_super_depth == 7);
        }
        if (!mode) assert(out.type == FVM_VALUE_OBJECT && out.object->resource_result.object->properties["depth"].number == expected_depth);
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_owner && !c->calling_setter_owner && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH && !c->exception_pending); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
