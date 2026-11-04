/* Native pointer-cell range exclusion; no managed roots or payload capability. */
#ifndef DVM_CELL_ACCESS_H
#define DVM_CELL_ACCESS_H
#include <stddef.h>
#include "disvm/object_guard.h"
#include "disvm/native_context.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DVM_CellDomain DVM_CellDomain; /* Stable caller-owned native cell-guard table. */


/* Begin never waits for an object owner while ACTIVE. */
typedef enum DVM_CellResult
{
    DVM_CELL_REFUSED = -1, /* Invalid geometry, context, role or access storage. */
    DVM_CELL_BUSY = 0, /* Required guard busy; begin rolls back, extend preserves its original access. */
    DVM_CELL_ACQUIRED = 1 /* All overlapping native pointer-cell guards are owned. */
} DVM_CellResult;


/* Zero before begin; stable noncopyable owner-local storage through end.
 * Fields are implementation metadata; never modify/copy a live access. */
typedef struct DVM_CellAccess
{
    DVM_CellDomain *domain; /* Borrowed native domain, pinned until end/rollback. */
    DVM_NativeContext *context; /* Borrowed exact bound owner while acquired. */
    DVM_ObjectGuard *guards[4]; /* Sorted unique guard stripes, never managed data. */
    unsigned int count; /* Number of unique stripes; metadata, not permission. */
    unsigned int held; /* Acquired prefix; private rollback state. */
} DVM_CellAccess;


/* Allocate an unpublished native table/shared parking domain. Return NULL on
 * native allocation failure with partial resources retired. No VM/root access. */
DVM_CellDomain *dvm_cell_domain_create(void);


/* Destroy empty QUIESCENT domain and clear *domain. Return0 unchanged for NULL,
 * current held guards or active accesses. Caller must exclude current/future
 * begin/end operations first: a counter cannot protect its own first lookup. */
int dvm_cell_domain_destroy(DVM_CellDomain **domain);


/* Claim both pointer-sized byte ranges, including unaligned/partly overlapping
 * slots. Caller independently keeps domain and mapped spans alive. Aligned cell
 * addresses hash into a fixed native table; collisions serialize independent
 * ranges but never grant ownership. No per-call allocation or object-owner wait.
 * A failed acquisition releases every guard/borrow; zero input stays zero.
 * Invalid nonzero access storage is refused unchanged.
 * Requires bound unguarded context and zero access; guard roles remain unchanged.
 * Metadata mutexes may wait. No VM/callback/managed retain or root acquisition.
 * Do not hold port registry mutex or keep access across VM/world/error calls. */
DVM_CellResult dvm_cell_access_begin(DVM_CellDomain *domain,
    const void *source, void *destination, DVM_CellAccess *access);


/* Same lifetime/role/rollback contract as pointer begin, with each span length
 * in1..sizeof(void*). Covers exact byte extents; no extra mapped bytes required.
 * Empty/oversized spans refuse unchanged. Useful for small scalar data cells. */
DVM_CellResult dvm_cell_access_spans_begin(DVM_CellDomain *domain,
    const void *source, size_t source_size, void *destination, size_t destination_size,
    DVM_CellAccess *access);


/* Release guards in reverse order, drop native domain borrow and zero access.
 * Return0 unchanged for foreign/unbound owner, nonacquired state or extra guard.
 * No VM/callback/payload read; caller retains ranges/domain through return. Never
 * span native waits, scope/world changes or arbitrary native/OS unwinding. */
int dvm_cell_access_end(DVM_CellAccess *access);
#ifdef __cplusplus
}
#endif
#endif
