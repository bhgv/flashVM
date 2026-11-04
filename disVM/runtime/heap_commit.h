/* Conditional owner publication into an independently rooted managed slot. */
#ifndef DVM_HEAP_COMMIT_H
#define DVM_HEAP_COMMIT_H
#include "heap_hold.h"


/* [GC:] cell is a mapped owning reference slot in a live GC-rooted container/
 * frame. observed/incoming are distinct exclusively owned successful Heap-holds or displaced commit leases;
 * their active one-entry reservations must not be externally altered. retired is
 * zeroed private output. All control spans/cell are disjoint; no lease is copied.
 * Require ACTIVE/world and container/frame/referent/code/GC lifetime. Compare
 * pointer identity under the cell guard, not RC1 or an unrooted address token.
 * Return1: incoming owner moves into cell, displaced slot owner/reservation into
 * retired, incoming is cleared, observed unchanged. Return0: stale value, every
 * input/output unchanged. Return-1: role/geometry/guard/mark/root refusal unchanged
 * except monotone mark metadata. Retarget refusal rolls slot publication back
 * before unlock. No allocation/free/managed retain/release/destructor/VM callback inside.
 * Caller retires leases AFTER return. This is not general concurrent allocator/
 * collector admission, a history/version CAS or a root snapshot lease. */
int dvm_runtime_heap_commit(void *cell, const DVM_HeapHold *observed,
    DVM_HeapHold *incoming, DVM_HeapHold *retired);
#endif
