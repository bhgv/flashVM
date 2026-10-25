/* FlashVM: execute a prepared native script body with script activation semantics. */
#include "native_body.hpp"
#include "../../core/this_cell.hpp"
#include "support.hpp"
#include "execution_roots.h"
#include "../target_scope.hpp"
#include "../../core/exception_internal.hpp"
#include "../../core/finalizer_entry.hpp"


/* [GC:] Preserve script identity and a snapshotted capture across GC and self-unbinding. */
FVM_Result fvm_exec_native_body(FVM_Context *c,
                                const FVM_ScriptBodyEntry &entry,
                                const FVM_ScriptFrame &frame,
                                FVM_Value *out,
                                const fvm_detail::vector<fvm_detail::string> *pool,
                                const FVM_Value *this_value)
{
    /* Raw pool bytes belong to the activation snapshot, not the managed graph.
     * Nested native functions/root entries must restore the caller's borrowed view. */
    struct PoolScope
    {
        FVM_Context *context; /* Owning thread's transient native execution environment. */
        const fvm_detail::vector<fvm_detail::string> *previous; /* Suspended caller snapshot. */


        /* Publish the current snapshot, including null for a fresh root entry. */
        PoolScope(FVM_Context *c, const fvm_detail::vector<fvm_detail::string> *value)
            : context(c), previous(c->execution_mode.native_pool)
        {
            context->execution_mode.native_pool = value;
        }


        /* Restore the caller view through success, exception and allocation failure. */
        ~PoolScope()
        {
            context->execution_mode.native_pool = previous;
        }
    } pool_scope(c, pool);


    FVM_ExecutionRoots roots(c, FVM_NATIVE_BODY_ROOT_COUNT);
    FVM_Object *objects[FVM_NATIVE_BODY_ROOT_COUNT];
    objects[FVM_NATIVE_BODY_CAPTURE_ROOT] = entry.capture;
    objects[FVM_NATIVE_BODY_THIS_ROOT] = frame.implicit_this;
    objects[FVM_NATIVE_BODY_PREVIOUS_THIS_ROOT] = c->script_this;
    for (size_t i = 0; i < FVM_NATIVE_BODY_ROOT_COUNT; ++i)
    {
        if (objects[i])
        {
            roots.values[i].type = FVM_VALUE_OBJECT;
            roots.values[i].object = objects[i];
        }
    }
    FVM_Value prepared_this;
    fvm_value_undefined(&prepared_this);
    if (frame.implicit_this)
    {
        prepared_this.type = FVM_VALUE_OBJECT;
        prepared_this.object = frame.implicit_this;
    }
    if (this_value)
    {
        prepared_this = *this_value;
    }


    /* [GC:] Retain current and suspended typed cells through all generated-body exits. */
    FVM_ThisCellScope identity(c, prepared_this);


    /* [GC:] Resource callbacks inspect receiver frames as well as traced values.
     * Publish physical and implicit receivers in interpreter order, including root JIT entries. */
    struct ReceiverScope
    {
        FVM_Context *context;     /* Context whose synchronous native activation is exposed. */
        FVM_GCRootFrame physical; /* [GC:] Physical receiver and suspended root-chain link. */
        FVM_GCRootFrame implicit; /* [GC:] Nearest implicit receiver, matching interpreter frames. */


        /* [GC:] Link only initialized frames; neither frame owns operand/local storage. */
        ReceiverScope(FVM_Context *c, FVM_Object *receiver) : context(c)
        {
            physical.previous = c->gc_frames;
            physical.stack = 0;
            physical.locals = 0;
            physical.receiver = receiver;
            implicit.previous = &physical;
            implicit.stack = 0;
            implicit.locals = 0;
            implicit.receiver = c->script_this;
            c->gc_frames = &implicit;
        }


        /* [GC:] Detach both frames before the typed-this scope restores the caller. */
        ~ReceiverScope()
        {
            context->gc_frames = physical.previous;
        }
    } receivers(c, frame.receiver);


    /* The shared activation builder already selected its definition timeline. */
    FVM_AVM1TargetScope target(c, frame.receiver, true);
    FVM_Value ignored;
    if (!out)
    {
        out = &ignored;
    }
    fvm_value_undefined(out);


    /* Generated bodies share the suspended interpreter's budget during nested calls. */
    struct BudgetScope
    {
        FVM_Context *context; /* Context whose execution nesting is scoped. */


        /* Preserve the outer instruction counter on native/script reentry. */
        explicit BudgetScope(FVM_Context *c) : context(c)
        {
            if (!c->avm1_execution_depth)
            {
                c->avm1_steps = 0;
            }
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
