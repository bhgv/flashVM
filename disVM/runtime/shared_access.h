/* Common hosted role preflight; not cell lifetime or mapping permission. */
#ifndef DVM_SHARED_ACCESS_H
#define DVM_SHARED_ACCESS_H


/* [GC:] Return0 before operands for refused context/role,1 for detached serial
 * inline mode,2 for acknowledged ACTIVE worker world. No managed reads, owner
 * acquisition, VM callbacks or allocation. Exact service checks follow at begin. */
int dvm_runtime_shared_mode(void);


/* Request a bounded replay for opcode after a rolled-back BUSY claim.
 * Called before any effects. Return one only inside the matching dispatch
 * with a live runtime process and remaining retry budget. */
int dvm_runtime_scalar_retry(int opcode);
#endif
