/* FlashVM: explicit native display-target resolution for C callers and generated code. */
#include "support.hpp"
#include "resolve_clip_constants.h"
/* Native instances bypass coercion; other values stringify exactly once before path lookup. */
extern "C" FVM_Result fvm_exec_resolve_clip(FVM_Context *c, FVM_Object *start,
                                            const FVM_Value *input, int allow_empty, FVM_Value *out)
{
    if (!c || c->destroying || c->gc_releasing || !out || !start || start->owner != c ||
        start->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(start) ||
        fvm_validate_value(c, input) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Publish roots before conversion or getters can execute script and collect. */
        FVM_ExecutionRoots roots(c, FVM_RESOLVE_CLIP_ROOT_COUNT);
        roots.values[FVM_RESOLVE_CLIP_START_ROOT].type = FVM_VALUE_OBJECT;
        roots.values[FVM_RESOLVE_CLIP_START_ROOT].object = start;
        roots.values[FVM_RESOLVE_CLIP_INPUT_ROOT] = *input;
        FVM_Object *clip = 0;
        if (input->type == FVM_VALUE_OBJECT &&
            input->object->resource_kind == FVM_RESOURCE_MOVIECLIP &&
            !fvm_movieclip_is_class_prototype(input->object))
            clip = input->object;
        else
        {
            FVM_Value &path = roots.values[FVM_RESOLVE_CLIP_PATH_ROOT];
            FVM_Result status =
                fvm_avm1_string(c, roots.values[FVM_RESOLVE_CLIP_INPUT_ROOT], &path);
            if (status != FVM_OK)
                return status;
            if (allow_empty || fvm_string_size(path))
            {
                status = fvm_movieclip_resolve_path(c, start, path, &clip);
                if (status != FVM_OK)
                    return status;
            }
        }
        FVM_Value result;
        fvm_value_undefined(&result);
        if (clip && clip->resource_kind == FVM_RESOURCE_MOVIECLIP &&
            !fvm_movieclip_is_class_prototype(clip))
        {
            result.type = FVM_VALUE_OBJECT;
            result.object = clip;
        }
        *out = result;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
