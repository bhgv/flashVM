/* Hosted Type-count conversion; not a general parallel managed heap API. */
#ifndef DVM_RUNTIME_TYPE_COUNTS_H
#define DVM_RUNTIME_TYPE_COUNTS_H


/* [GC:] Retain an already-owned live Type count. reference borrows aligned
 * native int storage. Refusal is a fatal host invariant, not a recoverable VM
 * error after partial allocation. No Type/payload reads or VM callback. Caller
 * still holds serialized heap admission and must not hold registry mutex. */
void dvm_runtime_type_retain(int *reference);


/* [GC:] Consume one Type owner. Return1 for unique LAST retirement,0 for LIVE;
 * refuse fatally on zero/negative. No storage access follows successful RMW:
 * another owner may retire immediately after a nonfinal release. No destructor
 * runs here. Type allocator/payload lifetime stays the caller's responsibility. */
int dvm_runtime_type_release(int *reference);


/* [GC:] Consume an owner whose immortal/static baseline must remain. Fatal on
 * refusal or LAST, without reading storage after RMW. reference is already
 * protected by caller; this does not establish a baseline or a GC root. */
void dvm_runtime_type_release_nonfinal(int *reference);


/* [GC:] Optional actual-runtime diagnostic; workers borrows live GC service.
 * Caller is ACTIVE hosted controller; waits with world admission released. */
struct DVM_GCWorkers;
void dvm_runtime_type_probe(struct DVM_GCWorkers *workers);
#endif
