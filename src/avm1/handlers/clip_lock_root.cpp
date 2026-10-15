/* FlashVM: intrinsic native root-lock state for C and generated code. */
#include "support.hpp"
#include "clip_lock_root_constants.h"
#include "../../standard/movieclip_lockroot.hpp"
#include "../../standard/movieclip_storage.hpp"
/* Query the intrinsic flag or atomically store versioned Boolean conversion of an explicit value.
 */
extern "C" FVM_Result fvm_exec_clip_lock_root(FVM_Context *c, int write, FVM_Object *clip,
                                              const FVM_Value *input, FVM_Value *out)
{
    if (!c || !out || c->destroying || c->gc_releasing || !clip || clip->owner != c ||
        clip->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(clip) ||
        (write && fvm_validate_value(c, input) != FVM_OK))
        return FVM_INVALID_ARGUMENT;
    try
    {
        if (!write)
        {
            bool locked = false;
            FVM_Result status = fvm_movieclip_root_locked(c, clip, &locked);
            if (status == FVM_OK)
                fvm_value_boolean(out, locked);
            return status;
        }
        /* [GC:] Retain instance/input while conversion and copied-map preparation may allocate. */
        FVM_ExecutionRoots roots(c, FVM_CLIP_LOCK_ROOT_ROOT_COUNT);
        roots.values[FVM_CLIP_LOCK_ROOT_CLIP_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_CLIP_LOCK_ROOT_CLIP_ROOT].object = clip;
        roots.values[FVM_CLIP_LOCK_ROOT_INPUT_ROOT] = *input;
        FVM_Result status = fvm_exec_to_boolean(c, &roots.values[FVM_CLIP_LOCK_ROOT_INPUT_ROOT],
                                                &roots.values[FVM_CLIP_LOCK_ROOT_VALUE_ROOT]);
        FVM_Object *data = 0;
        if (status == FVM_OK)
            status = fvm_movieclip_storage::state(clip, data);
        if (status != FVM_OK)
            return status;
        fvm_movieclip_storage::Properties next = data->properties;
        next["lock_root"] = roots.values[FVM_CLIP_LOCK_ROOT_VALUE_ROOT];
        /* [GC:] Publish prepared scalar state without a callback or allocation after commit. */
        data->properties.swap(next);
        *out = roots.values[FVM_CLIP_LOCK_ROOT_VALUE_ROOT];
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
