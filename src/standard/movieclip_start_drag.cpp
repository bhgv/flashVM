/* FlashVM: native MovieClip.startDrag adapter without encoded operand semantics. */
#include "movieclip_drag.hpp"
#include "movieclip_drag_constants.h"
#include "movieclip_state.hpp"
/* The class prototype has no instance; real clips share the C value-option preparation service. */
FVM_Result fvm_movieclip_start_drag(FVM_Context *c, FVM_Object *clip, unsigned int count,
                                    const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (fvm_movieclip_is_class_prototype(clip))
        return FVM_OK;
    unsigned int bounds = count > FVM_MOVIECLIP_DRAG_FIRST_BOUND_INDEX
                              ? count - FVM_MOVIECLIP_DRAG_FIRST_BOUND_INDEX
                              : 0;
    if (bounds > FVM_MOVIECLIP_DRAG_BOUND_ARGUMENTS)
        bounds = FVM_MOVIECLIP_DRAG_BOUND_ARGUMENTS;
    return fvm_exec_drag_values(c, clip, count ? args + FVM_MOVIECLIP_DRAG_CENTER_INDEX : 0, bounds,
                                bounds ? args + FVM_MOVIECLIP_DRAG_FIRST_BOUND_INDEX : 0);
}
