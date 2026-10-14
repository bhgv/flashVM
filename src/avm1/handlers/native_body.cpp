/* FlashVM: execute a prepared native script body with script activation semantics. */
#include "native_body.hpp"
#include "support.hpp"
#include "../target_scope.hpp"
#include "../../core/exception_internal.hpp"
#include "../../core/finalizer_entry.hpp"
/* [GC:] Preserve script identity and a snapshotted capture across GC and self-unbinding. */
FVM_Result fvm_exec_native_body(FVM_Context *c, const FVM_ScriptBodyEntry &entry,
                                const FVM_ScriptFrame &frame, FVM_Value *out)
{
    FVM_ExecutionRoots roots(c, 3);
    FVM_Object *objects[3] = {entry.capture, frame.implicit_this, c->script_this};
    for (size_t i = 0; i < 3; ++i)
        if (objects[i])
        {
            roots.values[i].type = FVM_VALUE_OBJECT;
            roots.values[i].object = objects[i];
        }
    /* Restore the caller's this even when generated code exits through an exception status. */
    struct ThisScope
    {
        FVM_Context *context; /* Owner of scoped implicit this. */
        FVM_Object *previous; /* [GC:] Suspended identity retained above. */
        /* Publish prepared this for nested calls. */
        ThisScope(FVM_Context *c, FVM_Object *value) : context(c), previous(c->script_this)
        {
            c->script_this = value;
        }
        /* Restore identity on all exits. */
        ~ThisScope()
        {
            context->script_this = previous;
        }
    } identity(c, frame.implicit_this);
    /* The shared activation builder already selected its definition timeline. */
    FVM_AVM1TargetScope target(c, frame.receiver, true);
    FVM_Value ignored;
    if (!out)
        out = &ignored;
    fvm_value_undefined(out);
    /* Generated bodies share the suspended interpreter's budget during nested calls. */
    struct BudgetScope
    {
        FVM_Context *context; /* Context whose execution nesting is scoped. */
        /* Preserve the outer instruction counter on native/script reentry. */
        explicit BudgetScope(FVM_Context *c) : context(c)
        {
            if (!c->avm1_execution_depth)
                c->avm1_steps = 0;
            ++c->avm1_execution_depth;
        }
        /* Restore execution nesting on all exits. */
        ~BudgetScope()
        {
            --context->avm1_execution_depth;
        }
    } budget(c);
    c->debug_offset = 0;
    /* [GC:] Only consume an armed finalizer after every body-entry allocation succeeds. */
    fvm_finalizer_enter(c);
    /* [GC:] Script roots are active; generated bodies do not enter the native GC barrier.
     * Snapshot/validate borrowed result text before activation roots leave scope. */
    return fvm_native_result(c, entry.callback(c, &frame, out, entry.userdata), out);
}
