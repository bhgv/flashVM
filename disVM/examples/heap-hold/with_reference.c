/* Hosted C teaching helper: compile only with the matching DisVM ABI/objects. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "raise.h"
#include "heap_hold.h"


/* Stable native stack storage survives kprotect's body unwind. No field escapes. */
typedef struct HeldTask
{
    DVM_HeapHold hold; /* Owns one real root/strong reference after capture. */
    void (*body)(void *, void *); /* Synchronous borrowed callback, may raise a VM error. */
    void *user; /* Borrowed caller context, retained through body/cleanup. */
} HeldTask;


/* task is caller-owned stable storage with a live hold. Borrow its value only
 * for this synchronous callback; callback cannot keep the pointer after return.
 * Return1 on normal completion. A real VM error is caught by outer kprotect;
 * no source-cell guard spans the callback. No owner is transferred to body. */
static int invoke_task(void *argument)
{
    HeldTask *task = argument;
    /* The retained reference and GC root remain live while body runs. */
    task->body(task->hold.value, task->user);
    /* A normal callback has completed; zero is not a manufactured VM failure. */
    return 1;
}


/* [GC:] slot borrows a mapped owning cell; expected is a live native Type or
 * NULL for any genuine Heap value. allow_nil permits H, never a C NULL payload.
 * body/user are borrowed synchronously. Caller is ACTIVE, retains world/frame/
 * referent/code/GC lifetime, and holds no object guards. body may invoke fallible
 * VM services; it must not transfer/retain the borrowed value implicitly or keep
 * native pointers after return. Return void after consuming cleanup. Capture or
 * callback/retirement errors propagate through the real VM error mechanism. */
void dvm_example_with_heap_reference(const void *slot, Type *expected, int allow_nil,
    void (*body)(void *, void *), void *user)
{
    HeldTask task = {0};
    const char *problem;
    int completed;
    if (!body)
    {
        error(exInval);
    }
    /* Native metadata is reserved BEFORE any slot guard is acquired. */
    problem = dvm_runtime_heap_hold(slot, expected, allow_nil, &task.hold);
    if (problem)
    {
        /* Capture refused with no extra owner/root left behind and no guard. */
        error((char *)problem);
    }
    task.body = body;
    task.user = user;
    /* Use the actual protection result, not a guessed body_ok flag. */
    completed = kprotect(invoke_task, &task);
    /* Consumes the owner once and preserves the primary body diagnostic. */
    dvm_runtime_heap_finish(&task.hold, completed);
}
