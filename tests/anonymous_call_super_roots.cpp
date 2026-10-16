/* Anonymous CallMethod depth and capture roots survive removal of the function's last property. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include "function_adapter_super.cpp"
#include "../src/core/call_scope.hpp"
#include "../src/standard/super.hpp"
#include "flashvm/exception.h"
static FVM_Object *function, *capture;
static unsigned mode;
static FVM_Result body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    assert(frame->receiver == c->global && c->calling_function == function && !c->calling_owner && !c->calling_constructor && c->calling_super_depth == 0);
    int removed = 0; assert(fvm_object_delete(c, c->global, "anonymousFunction", &removed) == FVM_OK && removed);
    assert(fvm_object_set_prototype(c, c->global, 0) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
    bool alive = false; for (size_t i = 0; i < c->objects.size(); ++i) alive |= c->objects[i] == capture; assert(alive);
    if (mode == 1) { fvm_value_number(out, 77); return fvm_exception_raise(c, out); }
    if (mode == 2) return FVM_ERROR;
    return fvm_super_create(c, function, c->global, out);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (mode = 0; mode < 3; ++mode)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_construct(c, "Object", 0, 0, &capture) == FVM_OK);
        function = adapter_super_function(c, 0); assert(fvm_function_set_body(c, function, body, 0, capture) == FVM_OK);
        FVM_Value value = class_value(function), out, name; assert(fvm_set_global(c, "anonymousFunction", &value) == FVM_OK); fvm_value_string(&name, "");
        const FVM_Result expected = mode == 1 ? FVM_EXCEPTION : mode == 2 ? FVM_ERROR : FVM_OK;
        { FVM_CallScope outer(c, 0, 0, 0, 7);
          if (!route) { assert(fvm_exec_call_method(c, 0, &value, &name, c->global, 0, 0, &out) == expected); if (mode == 1) { assert(fvm_exception_get(c, &out) == FVM_OK && out.number == 77); assert(fvm_exception_clear(c) == FVM_OK); } }
          else { Program code; code.number(0); code.variable("anonymousFunction"); code.text(""); code.op(0x52); code.finish(); out = boxing_run(c, code, route - 1, expected); }
          assert(c->calling_super_depth == 7);
        }
        if (!mode) assert(out.type == FVM_VALUE_OBJECT && out.object->resource_result.object->properties["depth"].number == 0);
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_owner && !c->calling_constructor && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH && !c->exception_pending); fvm_context_destroy(c);
    }
    /* Unloaded host inference still has no super for a function without a defining owner. */
    FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
    FVM_Value owner = class_value(adapter_super_function(c, 0)), name, out; fvm_value_string(&name, "");
    assert(fvm_exec_call_method(c, 0, &owner, &name, c->global, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
