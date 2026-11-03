/* Actual Sys allocation ownership diagnostic; no guest API or wire changes. */
#ifndef DVM_RUNTIME_SERVICE_COUNTS_H
#define DVM_RUNTIME_SERVICE_COUNTS_H
struct DVM_GCWorkers;


/* [GC:] workers borrows live service. Caller remains ACTIVE except explicit
 * joined Fast intervals. Run real Sys allocation paths and check Type owners. */
void dvm_runtime_service_probe(struct DVM_GCWorkers *workers);
#endif
