/* FlashVM: explicit target resolution and dynamic removal shared by C and generated bodies. */
#include "support.hpp"
#include "remove_target_constants.h"
/* Native instances bypass string conversion; other values use versioned target-path coercion. */
extern "C" FVM_Result fvm_exec_remove_target(FVM_Context *c, FVM_Object *start,
                                             const FVM_Value *input)
{
    if (!c || c->destroying || c->gc_releasing || !start || start->owner != c ||
        start->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(start) ||
        fvm_validate_value(c, input) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Preserve explicit inputs before conversion can mutate globals and collect. */
        FVM_ExecutionRoots roots(c, FVM_REMOVE_TARGET_ROOT_COUNT);
        roots.values[FVM_REMOVE_TARGET_START_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_REMOVE_TARGET_START_ROOT].object = start;
        roots.values[FVM_REMOVE_TARGET_INPUT_ROOT] = *input;
        FVM_Object *clip = 0;
        FVM_Result status = FVM_OK;
        if (input->type == FVM_VALUE_OBJECT &&
            input->object->resource_kind == FVM_RESOURCE_MOVIECLIP &&
            !fvm_movieclip_is_class_prototype(input->object))
            clip = input->object;
        else
        {
            status = fvm_avm1_string(c, roots.values[FVM_REMOVE_TARGET_INPUT_ROOT],
                                     &roots.values[FVM_REMOVE_TARGET_PATH_ROOT]);
            if (status == FVM_OK)
                status = fvm_movieclip_resolve_path(
                    c, start, roots.values[FVM_REMOVE_TARGET_PATH_ROOT], &clip);
        }
        if (status != FVM_OK)
            return status;
        if (!clip || clip->resource_kind != FVM_RESOURCE_MOVIECLIP ||
            fvm_movieclip_is_class_prototype(clip))
            return FVM_OK;
        roots.values[FVM_REMOVE_TARGET_CLIP_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_REMOVE_TARGET_CLIP_ROOT].object = clip;
        return fvm_exec_remove_dynamic_clip(c, clip);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
