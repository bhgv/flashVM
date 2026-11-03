/* Optional real-heap ownership probe, not an alternative collector. */
#ifndef DVM_MANAGED_OWNER_PROBE_H
#define DVM_MANAGED_OWNER_PROBE_H
#include "disvm/gc_workers.h"

extern int dvm_managed_owner_probe_requested; /* Immutable startup diagnostic option. */


/* [GC:] Run after real GC start on its ACTIVE controller before helper launch.
 * world/workers borrowed through synchronous joins. Uses real managed String,
 * retained collector, root registry and native scopes. Failures terminate rather
 * than resume after broken ownership. No managed pointer crosses a VM boundary. */
void dvm_managed_owner_probe_run(DVM_NativeWorld *world, DVM_GCWorkers *workers);
#endif
