/* FlashVM: shared execution accounting and safe-point polling for interpreter/generated code. */
#include "../../core/runtime.hpp"
#include "flashvm/execution.h"
#include "flashvm/gc.h"
#include "flashvm/worker.h"
#include "../../jit/mode_internal.hpp"
#include <limits>
#include <new>
/* [GC:] Poll only where the caller has published every live managed value as a root. */
extern "C" FVM_Result fvm_exec_checkpoint(FVM_Context *c, unsigned long units,
                                          unsigned long location, unsigned int flags)
{
    if (!c || c->destroying || !c->avm1_execution_depth || (flags & ~FVM_EXEC_POLL))
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_debug_update_location(c, location);
        fvm_execution_trace_step(c, location);
        if (units > std::numeric_limits<unsigned long>::max() - c->avm1_steps)
            return FVM_ERROR;
        c->avm1_steps += units;
        if (c->runtime->config.instruction_limit &&
            c->avm1_steps > c->runtime->config.instruction_limit)
            return FVM_ERROR;
        if (!(flags & FVM_EXEC_POLL))
            return FVM_OK;
        int stopped = 0;
        if (c->worker && fvm_worker_stop_requested(c->worker, &stopped) == FVM_OK && stopped)
            return FVM_ERROR;
        /* [GC:] Finalizers keep the enclosing collection; native barriers retain GC's existing
         * gate. */
        return c->gc_collecting ? FVM_OK : fvm_gc_step(c, 0);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
