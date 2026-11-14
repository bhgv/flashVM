/* Copyright (c) 2026 bhgv
 * EN: Stage a coherent pool snapshot, then publish scalar output under VM guards.
 * RU: Снять coherent pool snapshot, затем опубликовать scalar output под VM guards.
 */
#include "pool.h"
#include "heap_pool.h"

extern Pool *heapmem; /* Runtime owns arena through all guest/native/collector joins. */


/* EN: Borrow rooted generated frame; stage native counters once before guarded
 * publication. Busy repeats publication only. No object walk, callbacks or retain.
 * RU: Заимствовать rooted generated frame; снять native counters до guarded
 * publication. Busy повторяет только publication. Без object walk/callback/retain. */
void Processctl_memorysnapshot(void *frame)
{
    F_Processctl_memorysnapshot call;
    LONG result[12] = {-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; /* Status, exact gauges and history/flags. */
    DVM_HeapPoolStats stats;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_memorysnapshot, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (dvm_heap_pool_snapshot(heapmem, &stats))
    {
        result[0] = 1;
        result[1] = (LONG)stats.arena_bytes;
        result[2] = (LONG)stats.header_bytes;
        result[3] = (LONG)stats.live_span;
        result[4] = (LONG)stats.free_span;
        result[5] = (LONG)stats.live_blocks;
        result[6] = (LONG)stats.free_blocks;
        result[7] = (LONG)stats.block_overhead;
        result[8] = (LONG)stats.allocations;
        result[9] = (LONG)stats.releases;
        result[10] = (LONG)stats.peak_live_span;
        result[11] = stats.flags;
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}
