/* Guarded constructor coordinator; no guest or serialized ABI. */
#ifndef DVM_POINTER_TRANSACTION_H
#define DVM_POINTER_TRANSACTION_H


/* [GC:] source/destination are independently mapped owning pointer cells in
 * live rooted VM frames/containers. They may alias. Require ACTIVE/NativeWorld,
 * no object guards, stable operand/frame/Type/code lifetime through callbacks.
 * Capture source once, build against held tail, conditionally publish and retire
 * outside guards. Stale tails discard/rebuild, at most eight preparations before
 * a VM error; intervening compliant writes are never overwritten blindly.
 * Monitor may inspect/raise, not retain private nodes, modify borrowed holds or
 * leave admission. Returns void after commit/cleanup, or raises after draining
 * every temporary lease, preserving the first body/cleanup error. A destructor
 * error after commit does not undo publication. No general parallel admission. */
void dvm_runtime_pointer_transaction(void *source, void *destination);
#endif
