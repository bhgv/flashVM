/* Rooted capture of one independently owned managed reference. */
#ifndef DVM_HEAP_HOLD_H
#define DVM_HEAP_HOLD_H
#include "root_batch.h"
#include "disvm/cell_access.h"

struct Type; /* Actual native managed descriptor, defined by interp.h. */


/* Zeroed stable owner-local lease. Do not copy a live lease or share it between
 * simultaneous callers. Explicit consuming transfer clears its source first. */
typedef struct DVM_HeapHold
{
    void *value; /* One retained managed owner, or H when explicitly allowed. */
    DVM_RootBatch *roots; /* One reserved group, active-empty for accepted H. */
} DVM_HeapHold;


/* [GC:] cell borrows a mapped owning slot and its genuine independently live
 * referent. expected borrows a live Type or NULL for any genuine managed type;
 * allow_nil accepts H only, never a C NULL stored value. hold must be zeroed.
 * Require ACTIVE/world admission. Prepare native roots before cell exclusion,
 * capture/retain/mark/publish while guarded, then unlock. Return static diagnostic
 * or NULL; failure leaves hold zero and undoes temporary retains/metadata.
 * No VM/destructor callback under guards. This is not arbitrary-pointer/type/
 * mapping validation, root-snapshot lease or concurrent GC admission. A live
 * hold does not exclude later payload access; mutable payload needs own guards. */
const char *dvm_runtime_heap_hold(const void *cell, struct Type *expected,
    int allow_nil, DVM_HeapHold *hold);


/* [GC:] Consume exactly one live hold outside guards under ACTIVE/world.
 * body_ok is an actual completed kprotect result. Preserve its primary VM error
 * across consuming retirement on failure; otherwise propagate retirement error.
 * Clear native storage before destructor, never retry a consumed owner. May
 * invoke real destructor/VM unwind. No asynchronous borrower may retain hold. */
void dvm_runtime_heap_finish(DVM_HeapHold *hold, int body_ok);


/* As for heap_hold, also report source admission in caller-private storage.
 * BUSY leaves the hold empty and destroys the unused root reservation. */
const char *dvm_runtime_heap_hold_attempt(const void *cell, struct Type *expected, int allow_nil,
    DVM_HeapHold *hold, DVM_CellResult *claim);
#endif
