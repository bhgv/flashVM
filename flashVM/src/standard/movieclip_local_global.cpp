/* FlashVM: native point adapter delegating to the shared bytecode-independent C service. */
#include "movieclip_point.hpp"
#include "movieclip_point_constants.h"
#include "flashvm/execution.h"


/* Prepare a missing value for loaded no-op policy and preserve undefined script returns. */
FVM_Result fvm_movieclip_local_global(
    FVM_Context *c, FVM_Object *clip, unsigned int count, const FVM_Value *args, FVM_Value *out, void *)
{
    FVM_Value missing;
    fvm_value_undefined(&missing);
    FVM_Result status = fvm_exec_clip_point(c, clip, 0, count ? args + FVM_MOVIECLIP_POINT_ARGUMENT_INDEX : &missing);
    if (status == FVM_OK)
    {
        fvm_value_undefined(out);
    }
    return status;
}
