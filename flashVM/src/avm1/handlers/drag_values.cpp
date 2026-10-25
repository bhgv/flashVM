/* FlashVM: script-style drag option preparation shared by native adapters and C/generated bodies.
 */
#include "support.hpp"
#include "drag_constants.h"


/* Convert optional center truth and forward positional limits before publishing a new drag. */
extern "C" FVM_Result fvm_exec_drag_values(
    FVM_Context *c, FVM_Object *clip, const FVM_Value *center, unsigned int count, const FVM_Value *bounds)
{
    if (!c || c->destroying || c->gc_releasing || !clip || clip->owner != c ||
        clip->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(clip) ||
        count > FVM_EXEC_DRAG_BOUND_LIMIT || (count && !bounds) || (center && fvm_validate_value(c, center) != FVM_OK))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (unsigned int i = 0; i < count; ++i)
    {
        if (fvm_validate_value(c, bounds + i) != FVM_OK)
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    try
    {


        /* [GC:] Keep every input, including the native receiver, through later collecting
         * callbacks. */
        FVM_ExecutionRoots roots(c, FVM_DRAG_VALUES_ROOT_COUNT);
        roots.values[FVM_DRAG_VALUES_CLIP_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_DRAG_VALUES_CLIP_ROOT].object = clip;
        if (center)
        {
            roots.values[FVM_DRAG_VALUES_CENTER_ROOT] = *center;
        }
        for (unsigned int i = 0; i < count; ++i)
        {
            roots.values[FVM_DRAG_VALUES_FIRST_BOUND_ROOT + i] = bounds[i];
        }
        FVM_Value flag;
        FVM_Result status = fvm_exec_to_boolean(c, &roots.values[FVM_DRAG_VALUES_CENTER_ROOT], &flag);
        if (status != FVM_OK)
        {
            return status;
        }
        FVM_ExecutionDragBounds limits;
        if (count)
        {
            double *values[] = {&limits.x_min, &limits.y_min, &limits.x_max, &limits.y_max};
            for (unsigned int i = 0; i < FVM_EXEC_DRAG_BOUND_LIMIT; ++i)
            {
                FVM_Value number;
                status = fvm_exec_to_number(c, 0, &roots.values[FVM_DRAG_VALUES_FIRST_BOUND_ROOT + i], &number);
                if (status != FVM_OK)
                {
                    return status;
                }
                *values[i] = number.number;
            }
        }
        return fvm_exec_drag_begin(c, clip, flag.boolean, count ? &limits : 0);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
