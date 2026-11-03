/* Managed roots retained behind native owner tickets; include interp.h first. */
#ifndef DVM_MANAGED_OWNER_H
#define DVM_MANAGED_OWNER_H
#include "disvm/owner_scope.h"
#include "disvm/cell_access.h"
#include "disvm/managed_reference.h"
#include "disvm/gc_workers.h"


/* [GC:] Initialize on the ACTIVE serialized controller after collector creation.
 * world/workers borrowed through close, drain, producer quiescence and finish.
 * Return 0 unchanged on invalid/in-use state. Native mutex is process-lifetime. */
int dvm_managed_owner_start(DVM_NativeWorld *world, DVM_GCWorkers *workers);


/* Native snapshot used for Fast requests; no managed memory is read. Return queue
 * length or zero when inactive. It does not reserve ownership or a shutdown pin. */
unsigned long dvm_managed_owner_pending(void);


/* [GC:] Drain at most budget tickets ONLY on the stopped-world collecting owner.
 * Return nonzero if more pending work, zero when empty; invalid collector role
 * is fatal before managed reads. Removes ptradd then uses retained destroy.
 * Budget counts top-level objects, not recursive destructor work. */
int dvm_managed_owner_drain(unsigned int budget);


/* Native preflight: return 1 iff no live ticket owners or producer callbacks.
 * Pending rooted retirements may remain. No admission or heap state changes. */
int dvm_managed_owner_can_close(void);


/* [GC:] Close capture admission on ACTIVE controller after sealing owner joins.
 * Return 0 unchanged with live owners/producers; rooted queued work remains for
 * Fast. Never destroy the world/workers while outstanding tickets exist. */
int dvm_managed_owner_close(void);


/* [GC:] After inactive wait-idle and producer joins, require exact empty state.
 * Return 0 without resetting service when owners/queue/producers remain. On
 * quiescence print optional diagnostics and clear borrowed world/workers.
 * Caller destroys collectors/world afterwards. No managed callbacks occur. */
int dvm_managed_owner_finish(int diagnostics);


/* Snapshot canonical native node count under service/registry locks; zero when
 * inactive. No lifetime reservation or managed data access; caller must not
 * hold the port registry mutex. */
unsigned long dvm_managed_owner_guard_entries(void);


/* [GC:] Actual canonical guard diagnostic on ACTIVE controller; workers is
 * borrowed service. All created managed owners retire before returning. */
void dvm_runtime_guard_registry_probe(DVM_GCWorkers *workers);


/* [GC:] Claim pointer-sized source/destination cells only on exact ACTIVE
 * managed service owner. Ranges are caller-owned/live; no object retain/root
 * is implied. Result and cleanup follow cell_access.h. No VM callbacks. */
DVM_CellResult dvm_managed_cells_begin(const void *source, void *destination,
    DVM_CellAccess *access);
#endif
