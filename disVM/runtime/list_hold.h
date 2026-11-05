/* Internal guarded list-slot capture; native metadata never grants provenance. */
#ifndef DVM_LIST_HOLD_H
#define DVM_LIST_HOLD_H
#include "root_batch.h"

struct List; /* Actual retained native list, defined by interp.h. */


/* Zeroed stable owner-local storage; do not copy a live hold. */
typedef struct DVM_ListHold
{
    struct List *value; /* One independent retained list owner after success. */
    DVM_RootBatch *roots; /* One actual Heap-header root through callbacks. */
} DVM_ListHold;


/* Capture cell under same cell/range domain as MOVP and scalar access. Caller
 * owns mapped source cell and its current referent; require ACTIVE worker role.
 * Prepare native root metadata before guards. Return NULL success or a static
 * exception with hold unchanged, guards released and extra retain undone.
 * Geometry/count/Tlist checks do not prove arbitrary-pointer/allocator lifetime.
 * No VM/error/destructor callback spans guards; native allocation may fail. */
/* A scheduled replay returns NULL with an empty hold. */
const char *dvm_runtime_list_hold(int opcode, const void *cell, DVM_ListHold *hold);


/* Finish one live hold outside guards. body_ok is the result of a completed real
 * kprotect: on failure preserve that error while consuming the temporary list
 * owner and freeing native root metadata. On success propagate a release error.
 * Never retry a consumed owner. May invoke destructor/VM error, not allocation
 * under guards. Caller retains ACTIVE admission; normal success returns void. */
void dvm_runtime_list_finish(DVM_ListHold *hold, int body_ok);
#endif
