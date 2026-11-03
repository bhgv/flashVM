/* Private checked C construction boundary; no serialized or guest API. */
#ifndef DVM_POINTER_BUILDER_H
#define DVM_POINTER_BUILDER_H


/* [GC:] Construct a pointer-list node and replace destination only on success.
 * Slots are mapped/owned/stable under caller's serialized managed admission;
 * complete source/tail VM roots survive allocation and monitor callbacks.
 * Slots may alias. Retains are checked at acquisition. VM errors roll back all
 * new owners and leave destination unchanged. The former destination owner
 * is consumed nonfinally while the new node keeps its distinct tail owner.
 * Monitor may inspect/raise but must not mutate slots or leave admission.
 * Not a leaf-JIT ABI, native async call, graph transfer or payload permission. */
void dvm_runtime_pointer_cons(void *source, void *destination);

struct REG;


/* [GC:] Shared C tail target for hosted SysV x64 pointer-cons entry. registers
 * must be this bound runtime's full-size ACTIVE REG; NULL/foreign/inactive
 * profiles decline0 before operand reads through the shared owner preflight.
 * Decoded PC/operands and complete roots are already published by C dispatch.
 * Return1 only after commit. VM errors propagate normally, never decline/retry
 * after partial work. Generated entry must tail-enter, leaving no JIT return PC.
 * Same source/admission/monitor restrictions as pointer_cons apply. */
int dvm_runtime_pointer_fast(struct REG *registers);

struct DVM_GCWorkers;


/* [GC:] Optional actual construction/alias/error/Fast diagnostic. workers is
 * borrowed live service; caller has ACTIVE admission except joined Fast waits. */
void dvm_runtime_pointer_builder_probe(struct DVM_GCWorkers *workers);
#endif
