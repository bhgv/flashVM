/* Resolver activation roots survive deletion, detached chains, collection and callback errors. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include "function_adapter_super.cpp"
#include "../src/standard/super.hpp"
#include "../src/core/call_scope.hpp"
#include "flashvm/exception.h"
static FVM_Object *receiver, *function, *capture;
static unsigned mode;
static FVM_Result body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    assert(frame->receiver == receiver && c->calling_function == function && !c->calling_owner && c->calling_super_depth == 0);
    int removed = 0; assert(fvm_object_delete(c, receiver, "__resolve", &removed) == FVM_OK && removed);
    assert(fvm_object_set_prototype(c, receiver, 0) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
    bool alive = false; for (size_t i = 0; i < c->objects.size(); ++i) alive |= c->objects[i] == capture; assert(alive);
    if (mode == 1) { fvm_value_number(out, 77); return fvm_exception_raise(c, out); }
    if (mode == 2) return FVM_ERROR;
    return fvm_super_create(c, function, receiver, out);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (mode = 0; mode < 3; ++mode)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK && fvm_construct(c, "Object", 0, 0, &capture) == FVM_OK);
        function = adapter_super_function(c, 0); assert(fvm_function_set_body(c, function, body, 0, capture) == FVM_OK);
        FVM_Value args[2], out; args[1] = class_value(function);
        assert(fvm_object_set(c, receiver, "__resolve", args + 1) == FVM_OK);
        args[0] = class_value(receiver); assert(fvm_set_global(c, "watchRootTarget", args) == FVM_OK);
        const FVM_Result expected = mode == 1 ? FVM_EXCEPTION : mode == 2 ? FVM_ERROR : FVM_OK;
        { FVM_CallScope outer(c, 0, 0, 0, 7);
          if (!route) { fvm_value_number(args, 81); assert(fvm_object_get(c, receiver, "slot", &out) == expected); if (mode == 1) { assert(fvm_exception_get(c, &out) == FVM_OK && out.number == 77); assert(fvm_exception_clear(c) == FVM_OK); } }
          else { Program code; code.variable("watchRootTarget"); code.text("slot"); code.op(0x4e); code.finish(); out = boxing_run(c, code, route - 1, expected); }
          assert(c->calling_super_depth == 7);
        }
        if (!mode) { assert(out.type == FVM_VALUE_OBJECT && out.object->resource_result.object->properties["depth"].number == 0); }
        assert(!c->resolve_depth && !c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_owner && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH && !c->exception_pending);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
