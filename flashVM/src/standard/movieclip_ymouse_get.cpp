/* FlashVM: native MovieClip._ymouse bridge to the shared local-pointer query. */
#include "movieclip_pointer.hpp"
#include "movieclip_state.hpp"


/* The class prototype has no display transform; instances read the context pointer locally. */
FVM_Result
fvm_movieclip_ymouse_get(FVM_Context *c, FVM_Object *clip, unsigned int, const FVM_Value *, FVM_Value *out, void *)
{
    if (fvm_movieclip_is_class_prototype(clip))
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    FVM_Value x, y;
    FVM_Result status = fvm_exec_clip_pointer(c, clip, &x, &y);
    if (status == FVM_OK)
    {
        *out = y;
    }
    return status;
}
