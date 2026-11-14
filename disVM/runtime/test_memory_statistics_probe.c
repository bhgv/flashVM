/* Copyright (c) 2026 bhgv
 * EN: Observe actual scalar pool snapshots and inject precise copied-output omissions.
 * RU: Наблюдать actual scalar pool snapshots и вводить точные copied-output omissions.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "heap_pool.h"
#include "thread_port.h"
#include "disvm/native_context.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int observed;        /* Atomic once-only production API/native unit evidence. */
static int scalar_observed; /* Atomic once-only actual wide tuple publication evidence. */
extern Pool *heapmem;       /* Production arena outlives all fixture guests. */
extern int __real_dvm_heap_pool_snapshot(Pool *, DVM_HeapPoolStats *);
extern void __real_dvm_sys_scalar_result(const void *, void *, size_t);


#include "memory/pool_statistics_probe.inc"


/* EN: Run the joined native oracle once, then call the real snapshot. Controls
 * corrupt only copied API output, never allocator state or ordinary guest work.
 * RU: Один раз выполнить joined native oracle, затем real snapshot. Controls
 * меняют только copied API output, не allocator state либо ordinary guest work. */
int __wrap_dvm_heap_pool_snapshot(Pool *pool, DVM_HeapPoolStats *output)
{
    int result;
    if (!__atomic_exchange_n(&observed, 1, __ATOMIC_RELAXED))
    {
        memory_unit();
    }
    result = __real_dvm_heap_pool_snapshot(pool, output);
    if (result && pool == heapmem)
    {
        if (CONTROL == 1)
        {
            output->free_span += 16;
        }
        if (CONTROL == 3)
        {
            return 0;
        }
        if (CONTROL == 2)
        {
            output->allocations = 0;
        }
    }
    return result;
}


/* EN/RU: Observe a real 96-byte result on a claimed owner, preserving production guards.
 * Наблюдать actual 96-byte result у claimed owner, сохраняя production guards. */
void __wrap_dvm_sys_scalar_result(const void *snapshot, void *destination, size_t size)
{
    if (size == 96)
    {
        DVM_NativeContext *context = dvm_native_context_current();
        require_memory(context && dvm_native_context_turn(context) && up && up->prog);
        if (!__atomic_exchange_n(&scalar_observed, 1, __ATOMIC_RELAXED))
        {
            fprintf(stderr, "NATIVEMEMORY scalarbytes=96 owner=1\n");
        }
    }
    __real_dvm_sys_scalar_result(snapshot, destination, size);
}
