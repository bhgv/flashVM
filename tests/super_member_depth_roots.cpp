/* Explicit-super selected method depth survives getter deletion and disconnected owner graphs. */
#define FVM_PROPERTY_CALLBACK_SUPER_FIXTURE_ONLY
#include "property_callback_super.cpp"
#include "../src/core/call_scope.hpp"
#include "../src/standard/super.hpp"
#include "flashvm/exception.h"
static FVM_Object *receiver, *owner, *method, *capture;
static unsigned expected_depth, mode;
static FVM_Result getter_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    int removed = 0; assert(fvm_object_delete(c, owner, "slot", &removed) == FVM_OK && removed);
    assert(fvm_object_set_prototype(c, receiver, 0) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
    *out = class_value(frame->capture); return FVM_OK;
}
static FVM_Result method_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    assert(frame->receiver == receiver && c->calling_owner == owner && c->calling_function == method && c->calling_super_depth == expected_depth);
    assert(fvm_gc_collect(c, 0) == FVM_OK); bool owner_live = false, capture_live = false;
    for (size_t i = 0; i < c->objects.size(); ++i) { owner_live |= c->objects[i] == owner; capture_live |= c->objects[i] == capture; } assert(owner_live && capture_live);
    if (mode == 1) { fvm_value_number(out, 77); return fvm_exception_raise(c, out); }
    if (mode == 2) return FVM_ERROR;
    return fvm_super_create(c, method, receiver, out);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned base = 0; base < 3; ++base)
    for (unsigned location = 0; location < 3; ++location)
    for (mode = 0; mode < 3; ++mode)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *nodes[7]; for (unsigned i = 0; i < 7; ++i) assert(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK);
        receiver = nodes[0]; owner = nodes[base + location + 1]; expected_depth = base + location + 1;
        assert(fvm_gc_pin(c, receiver) == FVM_OK && fvm_construct(c, "Object", 0, 0, &capture) == FVM_OK);
        method = adapter_super_function(c, 0); assert(fvm_function_set_body(c, method, method_body, 0, capture) == FVM_OK);
        FVM_Object *getter = adapter_super_function(c, 0); assert(fvm_function_set_body(c, getter, getter_body, 0, method) == FVM_OK);
        callback_super_property(c, owner, "slot", 4, getter, version_function(c, 0));
        for (unsigned i = 0; i < 7; ++i) assert(fvm_object_set_prototype(c, nodes[i], i < 6 ? nodes[i + 1] : 0) == FVM_OK);
        FVM_Value value, out; { FVM_CallScope scope(c, 0, method, 0, base); assert(fvm_super_create(c, method, receiver, &value) == FVM_OK); } FVM_Object *target = value.object; assert(fvm_set_global(c, "capturedMemberTarget", &value) == FVM_OK);
        FVM_Result expected = mode == 1 ? FVM_EXCEPTION : mode == 2 ? FVM_ERROR : FVM_OK;
        { FVM_CallScope outer(c, 0, 0, 0, 7);
          if (!route) { assert(fvm_object_call(c, target, "slot", 0, 0, &out) == expected); if (mode == 1) { assert(fvm_exception_get(c, &out) == FVM_OK && out.number == 77); assert(fvm_exception_clear(c) == FVM_OK); } }
          else { Program code; code.number(0); code.variable("capturedMemberTarget"); code.text("slot"); code.op(0x52); code.finish(); out = boxing_run(c, code, route - 1, expected); }
          assert(c->calling_super_depth == 7);
        }
        if (!mode) assert(out.type == FVM_VALUE_OBJECT && out.object->resource_result.object->properties["depth"].number == expected_depth);
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_owner && !c->calling_setter_owner && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH && !c->exception_pending); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
