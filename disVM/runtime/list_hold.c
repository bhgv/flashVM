/* Preserve the list-specific contract through the common rooted owner lease. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "raise.h"
#include "list_hold.h"
#include "heap_hold.h"
#include "shared_access.h"
#include <stdlib.h>


/* [GC:] cell/hold retain the existing owning Tlist-only, nonnil contract.
 * Common capture supplies identical roots/retain/exclusion; transfer its native
 * lease fields explicitly after success. No throwing work follows transfer.
 * Return static error/NULL with caller storage unchanged on refusal. */
const char *dvm_runtime_list_hold(int opcode, const void *cell, DVM_ListHold *hold)
{
    DVM_HeapHold captured = {0};
    const char *problem;
    DVM_CellResult claim;
    if (!hold || hold->value || hold->roots)
    {
        return exHeap;
    }
    problem = dvm_runtime_heap_hold_attempt(cell, &Tlist, 0, &captured, &claim);
    if (problem)
    {
        return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode) ? NULL : problem;
    }
    hold->value = captured.value;
    hold->roots = captured.roots;
    captured.value = NULL;
    captured.roots = NULL;
    return NULL;
}


/* [GC:] Exclusively consume the live list lease into synchronous local storage.
 * Clear caller fields before real retirement/unwind. body_ok is the completed
 * kprotect result; common finish preserves its primary error and consumes once.
 * No callbacks can retain this stack-local lease after return/unwind. */
void dvm_runtime_list_finish(DVM_ListHold *hold, int body_ok)
{
    DVM_HeapHold captured = {0};
    if (!hold || !hold->value || !hold->roots)
    {
        abort();
    }
    captured.value = hold->value;
    captured.roots = hold->roots;
    hold->value = NULL;
    hold->roots = NULL;
    dvm_runtime_heap_finish(&captured, body_ok);
}
