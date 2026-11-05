/* Common hosted role preflight; not cell lifetime or mapping permission. */
#ifndef DVM_SHARED_ACCESS_H
#define DVM_SHARED_ACCESS_H
#include <stddef.h>


/* [GC:] Return0 before operands for refused context/role,1 for detached serial
 * inline mode,2 for acknowledged ACTIVE worker world. No managed reads, owner
 * acquisition, VM callbacks or allocation. Exact service checks follow at begin. */
int dvm_runtime_shared_mode(void);


/* Request a bounded replay for opcode after a rolled-back BUSY claim.
 * Called before any effects. Return one only inside the matching dispatch
 * with a live runtime process and remaining retry budget. */
int dvm_runtime_scalar_retry(int opcode);


/* Check whether opcode owns the armed experimental dispatch and the borrowed
 * byte spans are scalar cells in its rooted current frame. Zero third size
 * ignores its address. No heap release. */
int dvm_runtime_parallel_scalar_spans(int opcode, const void *first, size_t first_size,
    const void *second, size_t second_size, const void *third, size_t third_size);


extern int dvm_runtime_parallel_real; /* Startup-only private REAL evaluation mode. */

/* Try a native lease for private FP evaluation. The checked scalar spans
 * belong to the armed current frame. No guard may be live. Return zero for
 * inline evaluation or one for a matching FP reentry. */
int dvm_runtime_parallel_real_leave(int opcode, const void *first, size_t first_size,
    const void *second, size_t second_size, const void *third, size_t third_size);

/* Restore NativeWorld while preserving the evaluator's native FP environment.
 * released is the matching result from dvm_runtime_parallel_real_leave. */
void dvm_runtime_parallel_real_enter(int released);


/* Check that target is the current decoded instruction's immediate cell.
 * The dispatch retains its module throughout an audited native lease. */
int dvm_runtime_instruction_target(const void *target);
#endif
