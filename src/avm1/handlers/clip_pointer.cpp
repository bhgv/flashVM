/* FlashVM: explicit clip-local pointer projection for C callers and generated bodies. */
#include "support.hpp"
#include "clip_pointer_constants.h"
#include "../../core/scene_numeric.hpp"
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
        const bool loaded = c->swf_loaded;
        double pointer_x = c->pointer_world_x, pointer_y = c->pointer_world_y;
        /* [GC:] Geometry getters run script under the same native barrier as other clip queries. */
        FVM_ExecutionRoots roots(c, FVM_CLIP_POINTER_ROOT_COUNT);
        roots.values[FVM_CLIP_POINTER_RECEIVER_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_CLIP_POINTER_RECEIVER_ROOT].object = clip;
        FVM_NativeCallGuard guard(c);
        if (loaded)
        {
            /* The current scene uses a unit device-pixel ratio. Round the global pointer
             * to device pixels before applying the inverse local-twip projection. */
            FVM_TwipMatrix world, projection, inverse;
            FVM_TwipMatrix identity = {1, 0, 0, 1, 0, 0};
            FVM_TwipMatrix pixels = {FVM_CLIP_POINTER_PIXEL_SCALE, 0, 0,
                                     FVM_CLIP_POINTER_PIXEL_SCALE, 0, 0};
            FVM_Result status = fvm_exec_clip_world_matrix(c, clip, &world);
            if (status == FVM_OK)
                status = fvm_matrix_multiply_twips(&pixels, &world, &projection);
            if (status != FVM_OK)
                return status;
            status = fvm_matrix_inverse_twips(&projection, &inverse);
            if (status == FVM_INVALID_ARGUMENT)
            {
                /* A singular mouse projection uses identity in device-pixel space. */
                inverse = identity;
                status = FVM_OK;
            }
            if (status != FVM_OK)
                return status;
            long device_x = 0, device_y = 0, local_x = 0, local_y = 0;
            status = fvm_matrix_point_twips(&identity, fvm_scene_detail::input_twips(pointer_x),
                                            fvm_scene_detail::input_twips(pointer_y), &device_x,
                                            &device_y);
            if (status == FVM_OK)
                status = fvm_matrix_point_twips(&pixels, device_x, device_y, &device_x, &device_y);
            if (status == FVM_OK)
                status = fvm_matrix_point_twips(&inverse, device_x, device_y, &local_x, &local_y);
            if (status != FVM_OK)
                return status;
            fvm_value_number(out_x, (double)local_x / FVM_MATRIX_TWIPS_PER_PIXEL);
            fvm_value_number(out_y, (double)local_y / FVM_MATRIX_TWIPS_PER_PIXEL);
            return FVM_OK;
        }
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
