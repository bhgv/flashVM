/* FlashVM: world-space native point geometry shared by C callers and generated bodies. */
#include "support.hpp"
#include "clip_hit_constants.h"
#include "../../standard/movieclip_constants.h"


/* Test explicit world coordinates; root-relative conversion belongs to the caller. */
extern "C" FVM_Result
fvm_exec_clip_hit_point(FVM_Context *c, FVM_Object *clip, double x, double y, int shape, FVM_Value *out)
{
    if (!c || c->destroying || c->gc_releasing || !out || !clip || clip->owner != c ||
        clip->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(clip))
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Keep receiver alive; geometry getters may execute script with collection blocked.
         */
        FVM_ExecutionRoots roots(c, FVM_CLIP_HIT_POINT_ROOT_COUNT);
        roots.values[FVM_CLIP_HIT_RECEIVER_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_CLIP_HIT_RECEIVER_ROOT].object = clip;


        FVM_NativeCallGuard guard(c);
        double limit = std::numeric_limits<double>::max();
        if (!(x >= -limit && x <= limit && y >= -limit && y <= limit))
        {
            fvm_value_boolean(out, false);
            return FVM_OK;
        }
        FVM_PathBounds bounds;
        bool any = false, yes = false;
        FVM_Result status = fvm_movieclip_world_bounds(c, clip, bounds, any);
        if (status != FVM_OK)
        {
            return status;
        }
        yes = any && x >= bounds.x_min && x <= bounds.x_max && y >= bounds.y_min && y <= bounds.y_max;
        if (shape)
        {
            yes = false;
            if (x >= -FVM_MOVIECLIP_MAX_HIT_COORDINATE && x <= FVM_MOVIECLIP_MAX_HIT_COORDINATE &&
                y >= -FVM_MOVIECLIP_MAX_HIT_COORDINATE && y <= FVM_MOVIECLIP_MAX_HIT_COORDINATE)
            {
                status = fvm_movieclip_world_shape_hit(c, clip, x, y, yes);
            }
        }
        if (status == FVM_OK)
        {
            fvm_value_boolean(out, yes);
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
