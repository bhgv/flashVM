/* Copyright (c) 2026 bhgv
 * EN: Coherent native arena accounting; no payload reads or object ownership.
 * RU: Согласованный учёт native арены без чтения payload и владения объектами.
 */
#ifndef DVM_POOL_STATISTICS_H
#define DVM_POOL_STATISTICS_H
#include <stdint.h>


/* EN/RU: History overflow flags; exact current gauges never saturate.
 * Признаки переполнения истории; точные текущие gauges не насыщаются. */
enum
{
    DVM_POOL_ALLOCATIONS_SATURATED = 1, /* At least one allocation increment was clamped. */
    DVM_POOL_RELEASES_SATURATED = 2     /* At least one release increment was clamped. */
};


/* EN: One pool-mutex snapshot. Spans include native per-block overhead/alignment;
 * they are not requested/managed payload or OS resident bytes. All values fit big.
 * RU: Один snapshot под pool mutex. Spans включают native block overhead/alignment;
 * это не requested/managed payload либо OS resident bytes. Все значения входят в big. */
typedef struct DVM_HeapPoolStats
{
    uint64_t arena_bytes;    /* Reserved calloc arena extent, not process RSS. */
    uint64_t header_bytes;   /* Arena header outside its block partition. */
    uint64_t live_span;      /* Sum of all allocated block spans, including overhead/slack. */
    uint64_t free_span;      /* Sum of all free block spans; fragmentation is preserved. */
    uint64_t live_blocks;    /* Allocated native pool blocks, not necessarily published objects. */
    uint64_t free_blocks;    /* Free extents; this allocator does not coalesce them. */
    uint64_t block_overhead; /* Per-block native header/generation/tail bytes, excluding alignment. */
    uint64_t allocations;    /* Successful allocations, saturates at INT64_MAX. */
    uint64_t releases;       /* Successful frees, saturates at INT64_MAX. */
    uint64_t peak_live_span; /* Exact maximum simultaneously allocated span. */
    unsigned int flags;      /* Clamped history counters; gauges and partition remain exact. */
} DVM_HeapPoolStats;


struct Pool;


/* EN: Borrow live pool and disjoint native output through return. O(1), one pool
 * mutex; no allocation, callback, payload read or managed admission. Return1 for
 * coherent counters,0 with zero output on invalid/uninitialized/inconsistent state.
 * Caller keeps pool/mutex lifetime; concurrent pool destruction is not supported.
 * RU: Заимствовать live pool и отдельный native output до возврата. O(1), один
 * pool mutex; без allocation, callback, payload read либо managed admission.
 * Вернуть1 для coherent counters,0 с zero output при неверном/несогласованном
 * состоянии. Caller удерживает pool/mutex lifetime; concurrent destroy запрещён. */
int dvm_heap_pool_snapshot(struct Pool *pool, DVM_HeapPoolStats *output);
#endif
