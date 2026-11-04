/* Rooted preparation of list heads containing traced reference fields. */
#ifndef DVM_MAPPED_PREPARE_H
#define DVM_MAPPED_PREPARE_H
#include "heap_hold.h"
struct Type;


/* [GC:] source is a genuinely mapped owning record described by live immutable
 * type; each source reference has independent ownership/lifetime. tail is an
 * exclusive successful held Tlist/H, candidate zero disjoint output. Require
 * ACTIVE/NativeWorld, no object guards, stable Type/frame/code/source lifetime.
 * Capture bytes and retain mapped fields under complete source/destination range
 * guards; build/link/mark a complete list, publish candidate root, withdraw its
 * ordinary pin, then invoke monitor. Monitor may inspect/raise or perform separate
 * compliant application writes, not retain/mutate private node/holds or leave
 * admission. Return one complete rooted owner; on error output stays zero and
 * all acquired resources retire, preserving original error. Geometry is not
 * provenance. No independent opcode/ABI, arbitrary destructor/concurrent GC or
 * general parallel allocator admission. Stale dependent tail must rebuild. */
void dvm_runtime_mapped_prepare(const void *source, struct Type *type,
    const DVM_HeapHold *tail, DVM_HeapHold *candidate);
#endif
