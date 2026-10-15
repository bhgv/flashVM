/* FlashVM: native point adapter delegating to the shared bytecode-independent C service. */
#include "movieclip_point.hpp"
#include "movieclip_point_constants.h"
#include "flashvm/execution.h"
/* Preserve native missing-argument errors and undefined script return values. */
FVM_Result fvm_movieclip_local_global(FVM_Context *c, FVM_Object *clip, unsigned int count,
                                      const FVM_Value *args, FVM_Value *out, void *)
{
    if (!count)
        return FVM_INVALID_ARGUMENT;
    FVM_Result status = fvm_exec_clip_point(c, clip, 0, args + FVM_MOVIECLIP_POINT_ARGUMENT_INDEX);
    if (status == FVM_OK)
        fvm_value_undefined(out);
    return status;
}
