/* FlashVM: explicit native drag selection without argument coercion or bytecode. */
#include "support.hpp"
#include "drag_constants.h"


/* Normalize optional limits, capture the pointer and publish the new context-owned target. */
extern "C" FVM_Result
fvm_exec_drag_begin(FVM_Context *c, FVM_Object *clip, int center, const FVM_ExecutionDragBounds *bounds)
{
    if (!c || c->destroying || c->gc_releasing || !clip || clip->owner != c ||
        clip->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(clip))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_ExecutionDragBounds normalized = {0, 0, 0, 0};
    if (bounds)
    {
        normalized = *bounds;
        double *values[] = {&normalized.x_min, &normalized.y_min, &normalized.x_max, &normalized.y_max};
        double limit = std::numeric_limits<double>::max();
        for (unsigned int i = 0; i < FVM_DRAG_BOUND_COMPONENT_COUNT; ++i)
        {
            if (!(*values[i] >= -limit && *values[i] <= limit))
            {
                *values[i] = 0;
            }
        }
        if (normalized.x_max < normalized.x_min)
        {
            double temporary = normalized.x_max;
            normalized.x_max = normalized.x_min;
            normalized.x_min = temporary;
        }
        if (normalized.y_max < normalized.y_min)
        {
            double temporary = normalized.y_max;
            normalized.y_max = normalized.y_min;
            normalized.y_min = temporary;
        }
    }
    c->drag_bounds = normalized;
    c->drag_constrained = bounds != 0;
    c->drag_lock_center = center != 0;
    c->drag_last_world_x = c->pointer_world_x;
    c->drag_last_world_y = c->pointer_world_y;
    /* [GC:] The collector marks this context edge independently from script reachability. */
    c->drag_clip = clip;
    ++c->drag_revision;
    return FVM_OK;
}
