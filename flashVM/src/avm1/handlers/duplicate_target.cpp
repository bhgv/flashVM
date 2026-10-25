/* FlashVM: explicit clone requests shared by C callers, AVM1 and generated bodies. */
#include "support.hpp"
#include "duplicate_target_constants.h"
#include "../../standard/movieclip_constants.h"
#include "../../standard/movieclip_storage.hpp"


/* Convert depth/name before resolving source, keeping display depth separate from public depth. */
extern "C" FVM_Result fvm_exec_duplicate_target(FVM_Context *c,
                                                FVM_Object *start,
                                                const FVM_Value *source,
                                                const FVM_Value *name,
                                                const FVM_Value *depth,
                                                FVM_Value *out)
{
    if (!c || c->destroying || c->gc_releasing || !out || !start || start->owner != c ||
        start->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(start) ||
        fvm_validate_value(c, source) != FVM_OK || fvm_validate_value(c, name) != FVM_OK ||
        fvm_validate_value(c, depth) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Publish all original inputs before depth/name/source coercions can collect. */
        FVM_ExecutionRoots roots(c, FVM_DUPLICATE_TARGET_ROOT_COUNT);
        roots.values[FVM_DUPLICATE_TARGET_START_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_DUPLICATE_TARGET_START_ROOT].object = start;
        roots.values[FVM_DUPLICATE_TARGET_SOURCE_ROOT] = *source;
        roots.values[FVM_DUPLICATE_TARGET_NAME_ROOT] = *name;
        roots.values[FVM_DUPLICATE_TARGET_DEPTH_ROOT] = *depth;
        FVM_Result status = fvm_exec_to_number(
            c, 1, &roots.values[FVM_DUPLICATE_TARGET_DEPTH_ROOT], &roots.values[FVM_DUPLICATE_TARGET_DEPTH_ROOT]);
        if (status == FVM_OK)
        {
            status = fvm_avm1_string(
                c, roots.values[FVM_DUPLICATE_TARGET_NAME_ROOT], &roots.values[FVM_DUPLICATE_TARGET_TEXT_ROOT]);
        }
        if (status == FVM_OK)
        {
            status = fvm_exec_resolve_clip(c,
                                           start,
                                           &roots.values[FVM_DUPLICATE_TARGET_SOURCE_ROOT],
                                           1,
                                           &roots.values[FVM_DUPLICATE_TARGET_CLIP_ROOT]);
        }
        if (status != FVM_OK)
        {
            return status;
        }
        double absolute = roots.values[FVM_DUPLICATE_TARGET_DEPTH_ROOT].number;
        const FVM_Value &clip = roots.values[FVM_DUPLICATE_TARGET_CLIP_ROOT];
        if (clip.type == FVM_VALUE_OBJECT && absolute >= 0 && absolute <= FVM_MOVIECLIP_DUPLICATE_MAX_ABSOLUTE_DEPTH)
        {
            double public_depth = absolute - FVM_MOVIECLIP_DUPLICATE_DEPTH_BIAS;
            if (public_depth > FVM_MOVIECLIP_MAX_PUBLIC_DEPTH && fvm_movieclip_storage::parent(clip.object))
            {
                return FVM_UNSUPPORTED;
            }
            fvm_value_number(&roots.values[FVM_DUPLICATE_TARGET_DEPTH_ROOT], public_depth);
            status = fvm_exec_duplicate_clip(c,
                                             clip.object,
                                             &roots.values[FVM_DUPLICATE_TARGET_TEXT_ROOT],
                                             &roots.values[FVM_DUPLICATE_TARGET_DEPTH_ROOT],
                                             0,
                                             &roots.values[FVM_DUPLICATE_TARGET_RESULT_ROOT]);
        }
        if (status == FVM_OK)
        {
            *out = roots.values[FVM_DUPLICATE_TARGET_RESULT_ROOT];
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
