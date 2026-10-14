/* FlashVM: display property semantics for C callers and generated code. */
#include "support.hpp"
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
        FVM_ExecutionRoots roots(c, 6);
        roots.values[3] = *target_value;
        roots.values[4] = *index_value;
        if (write)
            roots.values[5] = *value;
        fvm_detail::vector<FVM_Value> &converted = roots.values;
        /* [GC:] A resolved temporary clip survives later valueOf/toString collection. */
        double number = 0;
        FVM_Result status = fvm_avm1_number(c, index_value, &number);
        if (status != FVM_OK)
            return status;
        FVM_Object *clip = 0;
        const FVM_Value &target = roots.values[3];
        if (target.type == FVM_VALUE_OBJECT &&
            target.object->resource_kind == FVM_RESOURCE_MOVIECLIP)
            clip = target.object;
        else
        {
            status = fvm_avm1_string(c, target, &converted[0]);
            if (status != FVM_OK)
                return status;
            FVM_Object *start = c->avm1_target_clip ? c->avm1_target_clip : c->avm1_base_clip;
            if (!start)
                return FVM_UNSUPPORTED;
            if (c->avm1_target_clip || fvm_string_size(converted[0]))
                status = fvm_movieclip_resolve_path(c, start, converted[0], &clip);
            if (status != FVM_OK)
                return status;
            if (clip && clip->resource_kind != FVM_RESOURCE_MOVIECLIP)
                clip = 0;
        }
        if (clip)
        {
            converted[2].type = FVM_VALUE_OBJECT;
            converted[2].object = clip;
        }
        bool valid = number == number && number > -1 && number < FVM_AVM1_PROPERTY_COUNT;
        unsigned int index = valid ? (unsigned int)(number < 0 ? 0 : std::floor(number)) : 0;
        FVM_Value result;
        fvm_value_undefined(&result);
        /* Player 7/8 policy: invalid targets and read-only fields do not coerce the value. */
        if (clip && valid)
        {
            if (!write)
                status = fvm_movieclip_get_action_property(c, clip, index, &result);
            else if (index == FVM_AVM1_PROPERTY_WIDTH || index == FVM_AVM1_PROPERTY_HEIGHT ||
                     index > FVM_AVM1_PROPERTY_NAME)
                status = FVM_UNSUPPORTED;
            else if (index != FVM_AVM1_PROPERTY_CURRENT_FRAME &&
                     index != FVM_AVM1_PROPERTY_TOTAL_FRAMES && index != FVM_AVM1_PROPERTY_TARGET &&
                     index != FVM_AVM1_PROPERTY_FRAMES_LOADED)
            {
                const FVM_Value &input = roots.values[5];
                if (index == FVM_AVM1_PROPERTY_NAME)
                {
                    if (input.type == FVM_VALUE_NUMBER && input.number != input.number &&
                        c->swf_loaded && c->swf_info.version >= FVM_SWF_VERSION_7)
                    {
                        FVM_Value zero;
                        fvm_value_number(&zero, 0);
                        status = fvm_avm1_string(c, zero, &converted[1]);
                    }
                    else
                        status = fvm_avm1_string(c, input, &converted[1]);
                }
                else if (input.type != FVM_VALUE_NULL && input.type != FVM_VALUE_UNDEFINED)
                {
                    double scalar = 0;
                    status = fvm_avm1_number(c, &input, &scalar);
                    fvm_value_number(&converted[1], scalar);
                }
                if (status == FVM_OK && converted[1].type != FVM_VALUE_UNDEFINED)
                    status = fvm_movieclip_set_action_property(c, clip, index, converted[1]);
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
