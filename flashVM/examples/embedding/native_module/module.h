/* Example C89 module interface, built on the existing FlashVM rooted callback API. */
#ifndef FLASHVM_EXAMPLE_NATIVE_MODULE_H
#define FLASHVM_EXAMPLE_NATIVE_MODULE_H
#include "flashvm/value.h"

#define DEMO_NATIVE_CALL_LIMIT 1048576UL /* Keep the example's counter and arithmetic exact. */


/* Host-owned per-instance state. Keep it alive until its context is destroyed. */
typedef struct DemoNativeState
{
    unsigned long calls;    /* Successful operations; failed validation does not advance it. */
    double total;           /* Exact bounded integer sum accumulated by the AS property setter. */
    FVM_Object *seed_owner; /* [GC:] Borrowed object traced by every installed callback capture. */
} DemoNativeState;


/* Create an unpublished namespace for a fresh zero-initialized state and a same-
 * context seed_owner. Copies callback registrations; borrows state until teardown.
 * The namespace owns add/seed methods and the accumulate setter. On failure keep
 * *output unchanged and restore fresh state. Caller must immediately pin or publish
 * the returned object before another allocating VM operation. Owner thread only. */
FVM_Result demo_native_module_create(FVM_Context *context, DemoNativeState *state,
                                     FVM_Object *seed_owner, FVM_Object **output);
#endif
