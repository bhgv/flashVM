/* FlashVM: explicit target resolution and dynamic removal shared by C and generated bodies. */
#include "support.hpp"
#include "remove_target_constants.h"


/* Native instances bypass string conversion; other values use versioned target-path coercion. */
extern "C" FVM_Result fvm_exec_remove_target(FVM_Context *c, FVM_Object *start, const FVM_Value *input)
{
    if (!c || c->destroying || c->gc_releasing || !start || start->owner != c ||
        start->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(start) ||
        fvm_validate_value(c, input) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Preserve explicit inputs before conversion can mutate globals and collect. */
        FVM_ExecutionRoots roots(c, FVM_REMOVE_TARGET_ROOT_COUNT);
        roots.values[FVM_REMOVE_TARGET_START_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_REMOVE_TARGET_START_ROOT].object = start;
        roots.values[FVM_REMOVE_TARGET_INPUT_ROOT] = *input;
        FVM_Result status = fvm_exec_resolve_clip(
            c, start, &roots.values[FVM_REMOVE_TARGET_INPUT_ROOT], 1, &roots.values[FVM_REMOVE_TARGET_CLIP_ROOT]);
        if (status != FVM_OK)
        {
            return status;
        }
        const FVM_Value &target = roots.values[FVM_REMOVE_TARGET_CLIP_ROOT];
        if (target.type != FVM_VALUE_OBJECT)
        {
            return FVM_OK;
        }
        FVM_Object *clip = target.object;
        return fvm_exec_remove_dynamic_clip(c, clip);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
