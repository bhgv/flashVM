/* FlashVM: select target semantics for C callers and generated code. */
#include "support.hpp"
#include "execution_roots.h"
#include "../../standard/movieclip_scene.hpp"


/* Execute value semantics with validation and allocation failure containment. */
extern "C" FVM_Result fvm_exec_select_target(FVM_Context *c, const FVM_Value *input)
{
    if (!c || fvm_validate_value(c, input) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        FVM_ExecutionRoots roots(c, FVM_SELECT_TARGET_ROOT_COUNT);
        roots.values[FVM_SELECT_TARGET_INPUT_ROOT] = *input;
        FVM_Value &path = roots.values[FVM_SELECT_TARGET_PATH_ROOT];
        FVM_Result status = FVM_OK;
        FVM_Object *target = 0;
        bool reset = false, invalid = false;
        if (input->type == FVM_VALUE_UNDEFINED)
        {
            reset = c->swf_loaded && c->swf_info.version <= FVM_SWF_VERSION_6;
            invalid = !reset;
        }
        else
        {
            status = fvm_avm1_string(c, roots.values[FVM_SELECT_TARGET_INPUT_ROOT], &path);
        }
        if (status != FVM_OK)
        {
            return status;
        }
        if (!reset && !invalid)
        {
            reset = fvm_string_size(path) == 0;
        }
        if (reset)
        {
            target = c->avm1_base_clip;
        }
        else if (!invalid && c->avm1_base_clip)
        {
            status = fvm_movieclip_resolve_path(c, c->avm1_base_clip, path, &target);
            if (status != FVM_OK)
            {
                return status;
            }
            if (target && target->resource_kind != FVM_RESOURCE_MOVIECLIP)
            {
                target = 0;
            }
        }
        if (c->swf_loaded && target)
        {
            bool removed = false;
            status = fvm_movieclip_scene_removed(c, target, &removed);
            if (status != FVM_OK)
            {
                return status;
            }
            if (removed)
            {
                target = 0;
            }
        }
        /* [GC:] Adopt the selected clip before operand roots are released. */
        c->avm1_target_clip = target;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
