/* FlashVM: explicit query of the context-wide native drag target. */
#include "support.hpp"


/* [GC:] Return a borrowed object value retained by the context, or undefined when inactive. */
extern "C" FVM_Result fvm_exec_drag_target(FVM_Context *c, FVM_Value *out)
{
    if (!c || c->destroying || c->gc_releasing || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_undefined(out);
    if (c->drag_clip)
    {
        out->type = FVM_VALUE_OBJECT;
        out->object = c->drag_clip;
    }
    return FVM_OK;
}
