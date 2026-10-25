/* FlashVM: context-wide native drag completion, including deferred centered positioning. */
#include "support.hpp"


/* Flush once unless called from a geometry getter already updating; failures retain the drag. */
extern "C" FVM_Result fvm_exec_drag_end(FVM_Context *c)
{
    if (!c || c->destroying || c->gc_releasing)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!c->drag_updating)
    {
        FVM_Result status = fvm_exec_drag_update(c);
        if (status != FVM_OK)
        {
            return status;
        }
    }
    /* [GC:] Removing this root permits collection after temporary update scopes have unwound. */
    c->drag_clip = 0;
    ++c->drag_revision;
    return FVM_OK;
}
