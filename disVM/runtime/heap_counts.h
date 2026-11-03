/* Private checked Heap ownership; no parallel admission or payload authority. */
#ifndef DVM_RUNTIME_HEAP_COUNTS_H
#define DVM_RUNTIME_HEAP_COUNTS_H


/* [GC:] Retain an already-protected aligned native ulong count. reference must
 * remain live through acquisition. Fatal native invariant refusal; no guest
 * recovery after partial construction. No allocation/callback/marking or root.
 * Caller owns serialized admission and does not hold registry mutex. */
void dvm_runtime_heap_retain(unsigned long *reference);


/* [GC:] Consume one owner. Return1 for LAST,0 for LIVE; terminal refusal on
 * zero/invalid. Never access counter after successful RMW. LAST transfers only
 * retirement responsibility: caller still obeys gchalt, roots and collector
 * exclusion before destruction. Does not free memory or acquire admission. */
int dvm_runtime_heap_release(unsigned long *reference);


/* [GC:] Optional real-runtime diagnostic. workers borrows a live GC service;
 * caller is ACTIVE hosted controller and releases admission during waits. */
struct DVM_GCWorkers;
void dvm_runtime_heap_probe(struct DVM_GCWorkers *workers);


/* [GC:] Optional actual-opcode diagnostic. workers borrowed live service;
 * caller owns ACTIVE controller and saves/restores REG before leaving. */
void dvm_runtime_opcode_probe(struct DVM_GCWorkers *workers);
#endif
