/* FlashVM: explicit clip-local pointer projection for C callers and generated bodies. */
#include "support.hpp"
#include "clip_pointer_constants.h"
/* Snapshot pointer coordinates before matrix getters and publish both local values on success. */
extern "C" FVM_Result fvm_exec_clip_pointer(FVM_Context *c, FVM_Object *clip, FVM_Value *out_x,
                                            FVM_Value *out_y)
{
    if (!c || c->destroying || c->gc_releasing || !out_x || !out_y || out_x == out_y || !clip ||
        clip->owner != c || clip->resource_kind != FVM_RESOURCE_MOVIECLIP ||
        fvm_movieclip_is_class_prototype(clip))
        return FVM_INVALID_ARGUMENT;
    try
    {
        double pointer_x = c->pointer_world_x, pointer_y = c->pointer_world_y;
        /* [GC:] Geometry getters run script under the same native barrier as other clip queries. */
        FVM_ExecutionRoots roots(c, FVM_CLIP_POINTER_ROOT_COUNT);
        roots.values[FVM_CLIP_POINTER_RECEIVER_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_CLIP_POINTER_RECEIVER_ROOT].object = clip;
        FVM_NativeCallGuard guard(c);
        FVM_Affine world;
        FVM_Result status = fvm_movieclip_world_matrix(c, clip, world);
        if (status != FVM_OK)
            return status;
        double determinant = world.xx * world.yy - world.xy * world.yx;
        double limit = std::numeric_limits<double>::max();
        if (!determinant || !(determinant >= -limit && determinant <= limit))
            return FVM_INVALID_ARGUMENT;
        double dx = pointer_x - world.x0, dy = pointer_y - world.y0;
        double x = (world.yy * dx - world.xy * dy) / determinant;
        double y = (world.xx * dy - world.yx * dx) / determinant;
        if (!(x >= -limit && x <= limit && y >= -limit && y <= limit))
            return FVM_INVALID_ARGUMENT;
        fvm_value_number(out_x, x);
        fvm_value_number(out_y, y);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
