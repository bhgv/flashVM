/* FlashVM: intrinsic parent-space native dragging driven by the context world pointer. */
#include "drag_support.hpp"
#include "drag_constants.h"
#include "../../standard/movieclip_storage.hpp"
#include "../../standard/movieclip_scene.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/affine.h"
/* Transform pointer motion, clamp and publish position only after all allocating preparation. */
extern "C" FVM_Result fvm_exec_drag_update(FVM_Context *c)
{
    if (!c || c->destroying || c->gc_releasing || c->drag_updating)
        return FVM_INVALID_ARGUMENT;
    if (!c->drag_clip)
        return FVM_OK;
    FVM_DragUpdateGuard updating(c);
    try
    {
        FVM_Object *clip = c->drag_clip;
        unsigned long revision = c->drag_revision;
        double pointer_x = c->pointer_world_x, pointer_y = c->pointer_world_y;
        double last_x = c->drag_last_world_x, last_y = c->drag_last_world_y;
        bool center = c->drag_lock_center, constrained = c->drag_constrained;
        FVM_ExecutionDragBounds bounds = c->drag_bounds;
        /* [GC:] Retain original nodes even if a matrix getter replaces/ends the active drag. */
        FVM_ExecutionRoots roots(c, FVM_DRAG_UPDATE_ROOT_COUNT);
        roots.values[FVM_DRAG_CLIP_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_DRAG_CLIP_ROOT].object = clip;
        FVM_NativeCallGuard guard(c);
        bool removed = false;
        FVM_Result status = fvm_movieclip_scene_removed(c, clip, &removed);
        if (status != FVM_OK)
            return status;
        if (removed)
            return fvm_exec_drag_end(c);
        FVM_Object *parent = fvm_movieclip_storage::parent(clip);
        if (parent)
        {
            roots.values[FVM_DRAG_PARENT_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_DRAG_PARENT_ROOT].object = parent;
        }
        FVM_Affine local, world = {1, 0, 0, 1, 0, 0};
        status = fvm_movieclip_get_matrix(c, clip, &local);
        if (status == FVM_OK && parent)
            status = fvm_movieclip_world_matrix(c, parent, world);
        if (status != FVM_OK)
            return status;
        if (c->drag_revision != revision || c->drag_clip != clip)
            return FVM_OK;
        status = fvm_movieclip_scene_removed(c, clip, &removed);
        if (status != FVM_OK)
            return status;
        if (removed)
            return fvm_exec_drag_end(c);
        if (fvm_movieclip_storage::parent(clip) != parent)
            return FVM_OK;
        double x = 0, y = 0;
        status = fvm_affine_inverse_point(&world, center ? FVM_AFFINE_POINT : FVM_AFFINE_VECTOR,
                                          center ? pointer_x : pointer_x - last_x,
                                          center ? pointer_y : pointer_y - last_y, &x, &y);
        if (status != FVM_OK)
            return status;
        double limit = std::numeric_limits<double>::max();
        if (!center)
        {
            x += local.x0;
            y += local.y0;
        }
        if (!(x >= -limit && x <= limit && y >= -limit && y <= limit))
            return FVM_INVALID_ARGUMENT;
        if (constrained)
        {
            if (x < bounds.x_min)
                x = bounds.x_min;
            if (x > bounds.x_max)
                x = bounds.x_max;
            if (y < bounds.y_min)
                y = bounds.y_min;
            if (y > bounds.y_max)
                y = bounds.y_max;
        }
        if (!center)
        {
            double moved_x = x - local.x0, moved_y = y - local.y0;
            last_x += world.xx * moved_x + world.xy * moved_y;
            last_y += world.yx * moved_x + world.yy * moved_y;
            if (!(last_x >= -limit && last_x <= limit && last_y >= -limit && last_y <= limit))
                return FVM_INVALID_ARGUMENT;
        }
        local.x0 = x;
        local.y0 = y;
        status = fvm_movieclip_set_matrix(c, clip, &local);
        if (status == FVM_OK && !center)
        {
            c->drag_last_world_x = last_x;
            c->drag_last_world_y = last_y;
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
