/* A deleted length skips element getters and retains the detached list through target GC. */
#define FVM_APPLY_DELETED_LENGTH_FIXTURE_ONLY
#include "function_apply_deleted_length.cpp"
static unsigned deleted_index_calls, deleted_target_calls, deleted_fault;
static FVM_Result deleted_index(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *, void *)
{ ++deleted_index_calls; return FVM_ERROR; }
static FVM_Result deleted_target(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++deleted_target_calls; assert(frame->argc == 0 && frame->capture);
    int removed = 0; assert(fvm_object_delete(c, c->global, "deletedLengthList", &removed) == FVM_OK && removed);
    assert(fvm_object_delete(c, c->global, "deletedLengthFunction", &removed) == FVM_OK && removed);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    bool alive = false; for (size_t i = 0; i < c->objects.size(); ++i) alive |= c->objects[i] == frame->capture; assert(alive);
    fvm_value_number(out, 91);
    return deleted_fault == 1 ? FVM_ERROR : deleted_fault == 2 ? fvm_exception_raise(c, out) : FVM_OK;
}
static void deleted_index_getter(FVM_Context *c, FVM_Object *array)
{
    FVM_Object *fn = deleted_length_function(c); assert(fvm_function_set_body(c, fn, deleted_index, 0, 0) == FVM_OK);
    FVM_Value args[3], out; fvm_value_string(args, "0"); args[1] = class_value(fn); fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL;
    assert(fvm_object_call(c, array, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
}
#ifndef FVM_APPLY_DELETED_ROOTS_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (deleted_fault = 0; deleted_fault < 3; ++deleted_fault)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = deleted_length_array(c), *fn = deleted_length_function(c); deleted_index_getter(c, array);
        assert(fvm_function_set_body(c, fn, deleted_target, 0, array) == FVM_OK);
        FVM_Value value = class_value(fn), out; assert(fvm_set_global(c, "deletedLengthFunction", &value) == FVM_OK); value = class_value(array); assert(fvm_set_global(c, "deletedLengthList", &value) == FVM_OK);
        const FVM_Result expected = deleted_fault == 1 ? FVM_ERROR : deleted_fault == 2 ? FVM_EXCEPTION : FVM_OK;
        deleted_index_calls = deleted_target_calls = 0;
        if (!route) { FVM_Value args[2]; fvm_value_undefined(args); args[0].type = FVM_VALUE_NULL; args[1] = value; assert(fvm_object_call(c, fn, "apply", 2, args, &out) == expected); if (expected == FVM_EXCEPTION) assert(fvm_exception_clear(c) == FVM_OK); }
        else { Program p; p.variable("deletedLengthList"); p.op(0x96); p.op(1); p.op(0); p.op(2); p.number(2); p.variable("deletedLengthFunction"); p.text("apply"); p.op(0x52); p.finish(); out = boxing_run(c, p, route - 1, expected); }
        assert(deleted_target_calls == 1 && !deleted_index_calls && !c->exception_pending);
        if (expected == FVM_OK) assert(out.type == FVM_VALUE_NUMBER && out.number == 91);
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->script_this_cell); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
