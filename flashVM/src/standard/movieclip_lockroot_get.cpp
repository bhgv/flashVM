/* FlashVM: native MovieClip._lockroot getter bridge. */
#include "movieclip_lockroot.hpp"
#include "movieclip_state.hpp"
#include "flashvm/execution.h"
#include "../avm1/handlers/clip_lock_root_constants.h"
/* The class prototype has no display-instance flag; instances use the common C query. */
FVM_Result fvm_movieclip_lockroot_get(FVM_Context *c, FVM_Object *clip, unsigned int,
                                      const FVM_Value *, FVM_Value *out, void *)
{
    if (fvm_movieclip_is_class_prototype(clip))
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return fvm_exec_clip_lock_root(c, FVM_CLIP_LOCK_ROOT_READ, clip, 0, out);
}
