/* FlashVM: read-only C89 introspection of the active AVM1 display-target scope. */
#include "flashvm/avm1.h"
#include "../core/runtime.hpp"


/* Read borrowed target metadata without allocation, script entry or caller-output mutation on error. */
extern "C" FVM_Result fvm_avm1_get_target_info(FVM_Context *c, FVM_AVM1TargetInfo *out)
{
    if (!c || !out || out->struct_size < sizeof(*out) || c->destroying || c->gc_releasing)
    {
        return FVM_INVALID_ARGUMENT;
    }
    out->active = c->avm1_target_active ? 1 : 0;
    out->base_clip = c->avm1_base_clip;
    out->target_clip = c->avm1_target_clip;
    return FVM_OK;
}
