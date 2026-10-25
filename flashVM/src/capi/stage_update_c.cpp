/* FlashVM: allocation-free, context-local redraw requests for independently hosted event loops. */
#include "../core/runtime.hpp"
#include "flashvm/stage.h"

namespace
{


/* Prevent lifecycle-phase mutation/observation before touching outputs or context flags. */
bool usable(FVM_Context *c)
{
    return c && !c->destroying && !c->gc_collecting && !c->gc_releasing;
}
}


/* Coalesce explicit host requests; unlike updateAfterEvent, the host needs no script-event scope. */
extern "C" FVM_Result fvm_stage_request_update(FVM_Context *c)
{
    if (!usable(c))
    {
        return FVM_INVALID_ARGUMENT;
    }
    c->stage_update_pending = true;
    return FVM_OK;
}


/* Observe a pending request without consuming it or transferring any managed ownership. */
extern "C" FVM_Result fvm_stage_get_update(FVM_Context *c, int *requested)
{
    if (!usable(c) || !requested)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *requested = c->stage_update_pending ? 1 : 0;
    return FVM_OK;
}


/* Consume before drawing so another request made during host presentation remains pending. */
extern "C" FVM_Result fvm_stage_take_update(FVM_Context *c, int *requested)
{
    FVM_Result result = fvm_stage_get_update(c, requested);
    if (result == FVM_OK)
    {
        c->stage_update_pending = false;
    }
    return result;
}
