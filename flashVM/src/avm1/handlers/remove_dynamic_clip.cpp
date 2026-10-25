/* FlashVM: intrinsic dynamic-depth removal policy for C and generated code. */
#include "support.hpp"
#include "detach_clip_constants.h"
#include "../../standard/movieclip_storage.hpp"
#include "../../standard/movieclip_scene.hpp"


/* Filter native identity, depth and ancestor state before committing forced detachment. */
extern "C" FVM_Result fvm_exec_remove_dynamic_clip(FVM_Context *c, FVM_Object *clip)
{
    if (!c || c->destroying || c->gc_releasing || !clip || clip->owner != c ||
        clip->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(clip))
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Retain the instance and its native ancestry throughout eligibility/preparation. */
        FVM_ExecutionRoots roots(c, FVM_REMOVE_DYNAMIC_CLIP_ROOT_COUNT);
        roots.values[FVM_REMOVE_DYNAMIC_CLIP_INPUT_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_REMOVE_DYNAMIC_CLIP_INPUT_ROOT].object = clip;
        FVM_Object *data = 0;
        FVM_Result status = fvm_movieclip_storage::state(clip, data);
        long depth = 0;
        if (status == FVM_OK)
        {
            status = fvm_movieclip_storage::depth_field(data, depth);
        }
        if (status != FVM_OK)
        {
            return status;
        }
        if (depth < FVM_MOVIECLIP_MIN_DYNAMIC_DEPTH || !fvm_movieclip_storage::parent(clip))
        {
            return FVM_OK;
        }
        bool removed = false;
        status = fvm_movieclip_scene_removed(c, clip, &removed);
        if (status != FVM_OK || removed)
        {
            return status;
        }
        return fvm_movieclip_scene_remove(c, clip);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
