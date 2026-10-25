/* FlashVM: versioned updateAfterEvent records a host redraw request without rendering recursively. */
#include "stage_update.hpp"
#include "stage_update_constants.h"
#include "flashvm/stage.h"


/* Ignore unused arguments and out-of-event calls, preserving an already pending host request. */
FVM_Result
fvm_builtin_update_after_event(FVM_Context *c, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *out, void *)
{
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_undefined(out);
    if (!c || c->destroying || c->gc_collecting || c->gc_releasing)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (c->swf_loaded && c->swf_info.version < FVM_STAGE_UPDATE_MIN_SWF_VERSION)
    {
        return FVM_UNSUPPORTED;
    }
    return c->stage_update_allowed ? fvm_stage_request_update(c) : FVM_OK;
}
