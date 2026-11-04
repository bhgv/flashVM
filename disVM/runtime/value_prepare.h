/* Prepare unpublished list nodes with scalar or unmapped raw payloads. */
#ifndef DVM_VALUE_PREPARE_H
#define DVM_VALUE_PREPARE_H
#include "heap_hold.h"
#include <stddef.h>


/* [GC:] bytes is a stable private immutable snapshot of size unmapped bytes;
 * zero size ignores bytes. It must contain no managed references requiring
 * tracing/ownership. tail is a successful stable exclusive HeapHold (Tlist/H).
 * candidate is zeroed disjoint output; neither input may alias it. Caller keeps
 * ACTIVE/NativeWorld, Type/code/input lifetime and no object guards. Return void
 * with one rooted complete candidate owner; errors roll back every private stage
 * leaving candidate zero and inputs unchanged. Monitor sees coherent raw head,
 * tail H and ordinary root; may inspect/raise, not mutate/retain inputs/node or
 * leave admission. This is preparation only, not opcode/JIT or parallel GC
 * permission. After stale commit, discard/rebuild using the fresh held tail. */
void dvm_runtime_value_prepare(const void *bytes, size_t size,
    const DVM_HeapHold *tail, DVM_HeapHold *candidate);
#endif
