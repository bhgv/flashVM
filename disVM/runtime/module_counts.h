/* Private Module ownership conversion; registry/payload exclusion still required. */
#ifndef DVM_RUNTIME_MODULE_COUNTS_H
#define DVM_RUNTIME_MODULE_COUNTS_H


/* [GC:] Retain aligned native int count already protected by a Module owner or
 * excluded registry lookup. Fatal zero/negative/overflow invariant refusal.
 * No allocation, callback, root or metadata permission; do not hold registry mutex. */
void dvm_runtime_module_retain(int *reference);


/* [GC:] Consume one protected Module owner. Return1 for unique LAST,0 for LIVE;
 * native refusal terminates. Never read storage after successful RMW. LAST does
 * not itself unlink/free a module; retained unload keeps registry/MP/type cleanup
 * under serialized admission. No destructor/callback in this helper. */
int dvm_runtime_module_release(int *reference);


/* [GC:] Optional actual module/link/MP diagnostic. workers borrows live service;
 * ACTIVE controller releases admission only after publishing all owned roots. */
struct DVM_GCWorkers;
void dvm_runtime_module_probe(struct DVM_GCWorkers *workers);
#endif
