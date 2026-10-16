/* FlashVM: display property semantics for C callers and generated code. */
#include "support.hpp"
#include "display_property_constants.h"
#include "../display_property_ids.h"
/* Execute value semantics with validation and allocation failure containment. */
extern "C" FVM_Result fvm_exec_display_property(FVM_Context *c, int write,
                                                const FVM_Value *target_value,
                                                const FVM_Value *index_value,
                                                const FVM_Value *value, FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, target_value) != FVM_OK ||
        fvm_validate_value(c, index_value) != FVM_OK ||
        (write && fvm_validate_value(c, value) != FVM_OK))
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_DISPLAY_PROPERTY_ROOT_COUNT);
        roots.values[FVM_DISPLAY_PROPERTY_INPUT_TARGET_ROOT] = *target_value;
        roots.values[FVM_DISPLAY_PROPERTY_INPUT_INDEX_ROOT] = *index_value;
        if (write)
            roots.values[FVM_DISPLAY_PROPERTY_INPUT_VALUE_ROOT] = *value;
        fvm_detail::vector<FVM_Value> &converted = roots.values;
        /* [GC:] A resolved temporary clip survives later valueOf/toString collection. */
        double number = 0;
        FVM_Result status =
            fvm_avm1_number(c, &roots.values[FVM_DISPLAY_PROPERTY_INPUT_INDEX_ROOT], &number);
        if (status != FVM_OK)
            return status;
        FVM_Object *clip = 0;
        const FVM_Value &target = roots.values[FVM_DISPLAY_PROPERTY_INPUT_TARGET_ROOT];
        if (target.type == FVM_VALUE_OBJECT &&
            target.object->resource_kind == FVM_RESOURCE_MOVIECLIP)
            clip = target.object;
        else
        {
            status = fvm_avm1_string(c, target, &converted[FVM_DISPLAY_PROPERTY_TARGET_TEXT_ROOT]);
            if (status != FVM_OK)
                return status;
            FVM_Object *start = c->avm1_target_clip ? c->avm1_target_clip : c->avm1_base_clip;
            if (!start)
                return FVM_UNSUPPORTED;
            if (c->avm1_target_clip ||
                fvm_string_size(converted[FVM_DISPLAY_PROPERTY_TARGET_TEXT_ROOT]))
                status = fvm_movieclip_resolve_path(
                    c, start, converted[FVM_DISPLAY_PROPERTY_TARGET_TEXT_ROOT], &clip);
            if (status != FVM_OK)
                return status;
            if (clip && clip->resource_kind != FVM_RESOURCE_MOVIECLIP)
                clip = 0;
        }
        if (clip)
        {
            converted[FVM_DISPLAY_PROPERTY_CLIP_ROOT].type = FVM_VALUE_OBJECT;
            converted[FVM_DISPLAY_PROPERTY_CLIP_ROOT].object = clip;
        }
        bool valid = number == number && number > FVM_DISPLAY_PROPERTY_INDEX_MIN_NUMBER &&
                     number < FVM_AVM1_PROPERTY_COUNT;
        unsigned int index = valid ? (unsigned int)(number < 0 ? 0 : std::floor(number)) : 0;
        FVM_Value result;
        fvm_value_undefined(&result);
        /* Player 7/8 policy: invalid targets and read-only fields do not coerce the value. */
        if (clip && valid)
        {
            if (!write)
                status = fvm_movieclip_get_action_property(c, clip, index, &result);
            else if (index == FVM_AVM1_PROPERTY_XMOUSE || index == FVM_AVM1_PROPERTY_YMOUSE)
                status = FVM_OK; /* Read-only fields follow the existing legacy no-coercion policy. */
            else if (index > FVM_AVM1_PROPERTY_NAME)
                status = FVM_UNSUPPORTED;
            else if (index != FVM_AVM1_PROPERTY_CURRENT_FRAME &&
                     index != FVM_AVM1_PROPERTY_TOTAL_FRAMES && index != FVM_AVM1_PROPERTY_TARGET &&
                     index != FVM_AVM1_PROPERTY_FRAMES_LOADED)
            {
                const FVM_Value &input = roots.values[FVM_DISPLAY_PROPERTY_INPUT_VALUE_ROOT];
                if (index == FVM_AVM1_PROPERTY_NAME)
                {
                    if (input.type == FVM_VALUE_NUMBER && input.number != input.number &&
                        c->swf_loaded && c->swf_info.version >= FVM_SWF_VERSION_7)
                    {
                        FVM_Value zero;
                        fvm_value_number(&zero, 0);
                        status =
                            fvm_avm1_string(c, zero, &converted[FVM_DISPLAY_PROPERTY_VALUE_ROOT]);
                    }
                    else
                        status =
                            fvm_avm1_string(c, input, &converted[FVM_DISPLAY_PROPERTY_VALUE_ROOT]);
                }
                else if (input.type != FVM_VALUE_NULL && input.type != FVM_VALUE_UNDEFINED)
                {
                    double scalar = 0;
                    status = fvm_avm1_number(c, &input, &scalar);
                    fvm_value_number(&converted[FVM_DISPLAY_PROPERTY_VALUE_ROOT], scalar);
                }
                if (status == FVM_OK &&
                    converted[FVM_DISPLAY_PROPERTY_VALUE_ROOT].type != FVM_VALUE_UNDEFINED)
                    status = fvm_movieclip_set_action_property(
                        c, clip, index, converted[FVM_DISPLAY_PROPERTY_VALUE_ROOT]);
            }
        }
        if (status != FVM_OK)
            return status;
        *out = result;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
