/* Produce a rooted unpublished pointer-list candidate; no destination mutation. */
#ifndef DVM_POINTER_PREPARE_H
#define DVM_POINTER_PREPARE_H
#include "heap_hold.h"


/* [GC:] source/tail are stable exclusively borrowed successful Heap holds (H
 * allowed); tail is H or a genuine Tlist. candidate is zeroed disjoint output.
 * Require ACTIVE/world, frame/Type/referent/GC/code lifetime, no object guards.
 * Retain/build/link a complete node, publish its candidate root before removing
 * ordinary construction pin. Return void with one candidate owner/root, or raise
 * a real VM error after balanced private rollback leaving candidate zero.
 * Source/tail stay owned/unchanged. Monitor sees coherent head with tail=H and
 * may raise/inspect but not modify these holds or leave admission. No destination
 * cell or executable capability is published. A stale dependent tail must be
 * rebuilt before conditional commit. Native allocator remains serialized. */
void dvm_runtime_pointer_prepare(const DVM_HeapHold *source,
    const DVM_HeapHold *tail, DVM_HeapHold *candidate);
#endif
