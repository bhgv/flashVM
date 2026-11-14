/* Copyright (c) 2026 bhgv
 * EN: Captured builtin ownership boundaries and guarded scalar result publication.
 * RU: Captured builtin ownership boundaries и guarded scalar result publication.
 */


/* Internal captured Sys frame/result boundaries; no OS types or I/O here. */
#ifndef DVM_SYS_CAPTURE_H
#define DVM_SYS_CAPTURE_H
#include <stddef.h>

#define DVM_SYS_SCALAR_LIMIT 96U /* Bounded coherent scalar counter reply; existing range guards, no pointers. */
#define DVM_SYS_MAX_OWNERS 4U    /* Maximum jointly validated owners in current Sys results. */


/* Copy a complete builtin frame into aligned caller storage before callbacks.
 * frame/local are borrowed; size/ret_offset describe the matched generated frame,
 * result_bytes covers the complete result tuple. Reject nil/H or wrapping spans
 * through the VM error path. Original VM stack/roots must remain live throughout
 * the call; this creates no GC roots and proves no arbitrary address provenance. */
void dvm_sys_capture(const void *frame, void *local, size_t size, size_t ret_offset, size_t result_bytes);


/* Capture one previous managed owner from a checked slot before any callback.
 * Return H or a live referenced owner; reject nil, malformed heap prefix or zero
 * references. Does not retain/release it; caller keeps its mapped slot rooted. */
void *dvm_sys_owner(const void *slot);


/* Check a captured owner list, including reference multiplicity for aliasing slots.
 * owners/count are borrowed native metadata (1..MAX_OWNERS). All values must have
 * passed dvm_sys_owner. Raise exHeap before clearing any result if aliases would
 * consume more references than exist. No mutation, allocation or I/O. */
void dvm_sys_owners(void *const *owners, size_t count);


/* [GC:] EN: Publish a private pure-scalar snapshot into a captured live mapped
 * result, preserving shared-cell exclusion. Retry publication only on BUSY;
 * native effects must already be staged, never replayed. No pointer ownership.
 * RU: Публиковать private pure-scalar snapshot в captured live mapped result
 * под cell guards. После BUSY повторяется только publication, не native effect.
 * Pointer ownership этим API не передаётся. */
void dvm_sys_scalar_result(const void *snapshot, void *destination, size_t size);
#endif
