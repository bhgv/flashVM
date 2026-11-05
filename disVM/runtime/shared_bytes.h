/* Hosted byte copies; include matching native interp.h first. */
#ifndef DVM_SHARED_BYTES_H
#define DVM_SHARED_BYTES_H
#include <stddef.h>


extern int dvm_runtime_guarded_copies; /* Startup-only immutable selection before loading/JIT publication. */


/* [GC:] Copy caller-owned mapped source/destination bytes, preserving memmove
 * overlap semantics and exact bits. Return NULL after success or borrowed static
 * exception before payload effects. Zero length ignores addresses. Worker mode
 * claims the shared cell/range domain; inline mode stays serial. No allocation,
 * ownership retain/drop, VM callback or error occurs under guards. Raw bytes do
 * NOT acquire pointed-to ownership; typed compiler/VM lifetime premises remain. */
const char *dvm_runtime_bytes_copy(const void *source, void *destination, size_t size);
/* Borrow source and caller-private snapshot spans of size bytes.
 * Zero size ignores both addresses. Return NULL or a static diagnostic
 * after releasing source exclusion. */
const char *dvm_runtime_bytes_snapshot(const void *source, void *snapshot, size_t size);
/* Publish size bytes from caller-private storage into a live mapped destination.
 * Zero size ignores both addresses. Return a static error or NULL after unlock. */
const char *dvm_runtime_bytes_publish(const void *snapshot, void *destination, size_t size);
#endif
