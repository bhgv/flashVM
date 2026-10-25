/* FlashVM: native MovieClip._lockroot setter bridge. */
#include "movieclip_lockroot.hpp"
#include "movieclip_state.hpp"
#include "flashvm/execution.h"
#include "../avm1/handlers/clip_lock_root_constants.h"


/* Delegate versioned truth conversion and atomic state publication for native instances. */
FVM_Result fvm_movieclip_lockroot_set(
    FVM_Context *c, FVM_Object *clip, unsigned int argc, const FVM_Value *argv, FVM_Value *out, void *)
{
    if (!argc || fvm_movieclip_is_class_prototype(clip))
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return fvm_exec_clip_lock_root(c, FVM_CLIP_LOCK_ROOT_WRITE, clip, argv, out);
}
