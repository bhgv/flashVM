/* Internal captured Sys frame/result boundaries; no OS types or I/O here. */
#ifndef DVM_SYS_CAPTURE_H
#define DVM_SYS_CAPTURE_H
#include <stddef.h>

#define DVM_SYS_MAX_OWNERS 4U /* Maximum jointly validated owners in current Sys results. */


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
#endif
