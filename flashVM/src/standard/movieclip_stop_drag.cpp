/* FlashVM: native MovieClip.stopDrag adapter for context-wide completion. */
#include "movieclip_drag.hpp"
#include "movieclip_state.hpp"
/* Any native instance can end dragging; calling the class prototype remains an undefined no-op. */
FVM_Result fvm_movieclip_stop_drag(FVM_Context *c, FVM_Object *clip, unsigned int,
                                   const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (fvm_movieclip_is_class_prototype(clip))
        return FVM_OK;
    if (!clip || clip->owner != c || clip->resource_kind != FVM_RESOURCE_MOVIECLIP)
        return FVM_INVALID_ARGUMENT;
    return fvm_exec_drag_end(c);
}
