/* FlashVM: frame control semantics for C callers and generated code. */
#include "support.hpp"
#include "execution_roots.h"


/* Execute value semantics with validation and allocation failure containment. */
extern "C" FVM_Result fvm_exec_frame_control(
    FVM_Context *c, FVM_Object *clip, FVM_ExecutionFrame command, const FVM_Value *frame, int playing)
{
    if (!c || !clip || clip->owner != c || command < FVM_EXEC_NEXT_FRAME || command > FVM_EXEC_GOTO_FRAME ||
        (command == FVM_EXEC_GOTO_FRAME && fvm_validate_value(c, frame) != FVM_OK))
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        FVM_ExecutionRoots roots(c, FVM_FRAME_CONTROL_ROOT_COUNT);
        roots.values[FVM_FRAME_CONTROL_CLIP_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_FRAME_CONTROL_CLIP_ROOT].object = clip;
        if (command == FVM_EXEC_GOTO_FRAME)
        {
            roots.values[FVM_FRAME_CONTROL_FRAME_ROOT] = *frame;
        }
        return fvm_movieclip_frame_control(c,
                                           clip,
                                           command,
                                           command == FVM_EXEC_GOTO_FRAME ? &roots.values[FVM_FRAME_CONTROL_FRAME_ROOT]
                                                                          : 0,
                                           playing);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
