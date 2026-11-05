/* Hosted byte copies; include matching native interp.h first. */
#ifndef DVM_SHARED_BYTES_H
#define DVM_SHARED_BYTES_H
#include <stddef.h>
#include "disvm/cell_access.h"


extern int dvm_runtime_guarded_copies;
extern int dvm_runtime_parallel_byte_copies;
extern unsigned long dvm_runtime_parallel_byte_commits; /* Startup-only immutable selection before loading/JIT publication. */


/* [GC:] Copy caller-owned mapped source/destination bytes, preserving memmove
 * overlap semantics and exact bits. Return NULL after success or borrowed static
 * exception before payload effects. Zero length ignores addresses. Worker mode
 * claims the shared cell/range domain; inline mode stays serial. No allocation,
 * ownership retain/drop, VM callback or error occurs under guards. Raw bytes do
 * NOT acquire pointed-to ownership; typed compiler/VM lifetime premises remain. */
const char *dvm_runtime_bytes_copy(const void *source, void *destination, size_t size);
/* Same borrowed spans and copy semantics. An armed opcode can return NULL
 * after scheduling a replay on pre-effect BUSY. */
const char *dvm_runtime_bytes_copy_opcode(int opcode, const void *source, void *destination, size_t size);
/* Borrow source and caller-private snapshot spans of size bytes.
 * Zero size ignores both addresses. Return NULL or a static diagnostic
 * after releasing source exclusion. */
const char *dvm_runtime_bytes_snapshot(const void *source, void *snapshot, size_t size);
/* Publish size bytes from caller-private storage into a live mapped destination.
 * Zero size ignores both addresses. Return a static error or NULL after unlock. */
const char *dvm_runtime_bytes_publish(const void *snapshot, void *destination, size_t size);


/* Capture variants report native admission through required private claim
 * storage. BUSY guarantees no source read or output change. */
const char *dvm_runtime_bytes_snapshot_attempt(const void *source, void *snapshot,
    size_t size, DVM_CellResult *claim);
/* Capture two pointer-width-or-smaller spans under one guard union. A zero
 * second width ignores its address. The claim result is caller-private. */
const char *dvm_runtime_bytes_snapshot_pair_attempt(const void *first, size_t first_size,
    void *first_snapshot, const void *second, size_t second_size, void *second_snapshot,
    DVM_CellResult *claim);


/* Address-only publication for an armed pre-effect instruction. BUSY may
 * schedule a replay without writing destination. */
const char *dvm_runtime_bytes_publish_opcode(int opcode, const void *snapshot, void *destination, size_t size);


/* Snapshot two equal-width inputs and one pointer slot into private outputs.
 * BUSY can schedule a replay, leaving all snapshots untouched. */
const char *dvm_runtime_bytes_snapshot_operands(int opcode, const void *first, const void *second,
    size_t input_size, const void *slot, void *saved_first, void *saved_second,
    void *old, int *replay);
#endif
