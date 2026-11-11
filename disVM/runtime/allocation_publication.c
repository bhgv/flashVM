/* Copyright (c) 2026 bhgv
 * EN: Guarded destination capture and publication of rooted allocations.
 * RU: Защищённый захват назначения и публикация укоренённых объектов.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "raise.h"
#include "allocation_publication.h"
#include "heap_commit.h"
#include "thread_port.h"
#include "disvm/native_context.h"


/* [GC:] Capture borrowed destination into empty tail. The active caller
 * keeps the destination alive. BUSY releases all guards before GC polling.
 * Return NULL on capture or a static error with tail unchanged.
 * RU: Захватить заимствованное назначение в пустой tail. Вызывающий удерживает
 * его время жизни. При BUSY блокировки сняты до опроса GC.
 * Вернуть NULL при успехе или статическую ошибку без изменения tail. */
const char *dvm_allocation_hold_destination(void *destination, DVM_HeapHold *tail)
{
    DVM_CellResult claim;
    const char *problem;
    for (;;)
    {
        problem = dvm_runtime_heap_hold_attempt(destination, NULL, 1,
            tail, &claim);
        if (claim != DVM_CELL_BUSY || !dvm_native_context_poll())
        {
            return problem == exHeap ? "allocation destination guard refused" : problem;
        }
        fvm_port_thread_sleep_us(50);
    }
}


/* [GC:] Borrow destination and observed tail. Transfer rooted candidate to
 * the slot on success and return its displaced owner in retired.
 * BUSY preserves every hold while polling GC outside guards.
 * Return 1 on publication, 0 on stale observation, -1 on other refusal.
 * RU: При совпадении destination и tail передать candidate в ячейку,
 * вытесненную ссылку вернуть в retired. При BUSY удержания сохраняются.
 * Вернуть 1 при успехе, 0 при смене назначения, -1 при другом отказе. */
int dvm_allocation_commit_destination(void *destination, const DVM_HeapHold *tail,
    DVM_HeapHold *candidate, DVM_HeapHold *retired)
{
    for (;;)
    {
        DVM_CellResult claim;
        int result = dvm_runtime_heap_commit_attempt(destination, tail,
            candidate, retired, &claim);
        if (claim != DVM_CELL_BUSY || !dvm_native_context_poll())
        {
            return result;
        }
        fvm_port_thread_sleep_us(50);
    }
}
