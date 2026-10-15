/* FlashVM: intrinsic world-bounds overlap shared by C callers and generated bodies. */
#include "support.hpp"
#include "clip_hit_constants.h"
/* Compare stroked native bounds without looking up script hitTest/getBounds properties. */
extern "C" FVM_Result fvm_exec_clip_hit_object(FVM_Context *c, FVM_Object *clip, FVM_Object *target,
                                               FVM_Value *out)
{
    if (!c || c->destroying || c->gc_releasing || !out || !clip || clip->owner != c ||
        clip->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(clip) ||
        !target || target->owner != c || target->resource_kind != FVM_RESOURCE_MOVIECLIP ||
        fvm_movieclip_is_class_prototype(target))
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Both instances survive all geometry getters; copied paint edges need a barrier. */
        FVM_ExecutionRoots roots(c, FVM_CLIP_HIT_OBJECT_ROOT_COUNT);
        roots.values[FVM_CLIP_HIT_RECEIVER_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_CLIP_HIT_RECEIVER_ROOT].object = clip;
        roots.values[FVM_CLIP_HIT_TARGET_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_CLIP_HIT_TARGET_ROOT].object = target;
        FVM_NativeCallGuard guard(c);
        FVM_PathBounds first, second;
        bool any = false, has = false;
        FVM_Result status = fvm_movieclip_world_bounds(c, clip, first, any);
        if (status == FVM_OK)
            status = fvm_movieclip_world_bounds(c, target, second, has);
        if (status == FVM_OK)
            fvm_value_boolean(out, any && has && first.x_min <= second.x_max &&
                                       second.x_min <= first.x_max && first.y_min <= second.y_max &&
                                       second.y_min <= first.y_max);
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
