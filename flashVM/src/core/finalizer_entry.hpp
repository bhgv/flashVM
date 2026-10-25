/* [GC:] FlashVM: finalizer entry markers distinguish preparation failure from user-code execution.
 */
#ifndef FVM_CORE_FINALIZER_ENTRY_HPP
#define FVM_CORE_FINALIZER_ENTRY_HPP /* [GC:] Guard internal finalizer invocation state. */
#include "runtime.hpp"


/* [GC:] Commit once-only finalization when the first native callback or AVM1 body starts. */
inline void fvm_finalizer_enter(FVM_Context *context)
{     /* [GC:] Ordinary function calls have no armed finalizer and introduce no state change. */
    if (context->pending_finalizer)
    { /* [GC:] An entered callback/body must not be replayed after partial effects or failure. */
        context->pending_finalizer->finalized = true;
        /* [GC:] Nested calls belong to an already-started invocation, not a second finalization. */
        context->pending_finalizer = 0;
    }
}


/* [GC:] Arm a resolved invocation while preserving nested finalizer preparation state. */
class FVM_FinalizerScope
{
    FVM_Context *context_; /* [GC:] Owning VM context whose dispatch will signal user-code entry. */
    FVM_Object *object_;   /* [GC:] Managed object awaiting finalizer entry. */
    FVM_Object *previous_; /* [GC:] Previous armed finalizer restored on every exit. */

  public:


    /* [GC:] Preparation allocates no scope state and does not prematurely finalize the object. */
    FVM_FinalizerScope(FVM_Context *context, FVM_Object *object)
        : context_(context), object_(object), previous_(context->pending_finalizer)
    {
        context_->pending_finalizer = object;
    }


    /* [GC:] Restore the previous arm even when invocation preparation throws or returns OOM. */
    ~FVM_FinalizerScope()
    {
        context_->pending_finalizer = previous_;
    }


    /* [GC:] Retry only allocation failures before user code; terminal setup errors consume the
     * attempt. */
    FVM_Result complete(FVM_Result result)
    {
        if (result != FVM_OUT_OF_MEMORY)
        { /* [GC:] Terminal setup errors consume the attempt, while pre-entry OOM remains retryable. */
            object_->finalized = true;
        }
        return result;
    }
};
#endif
