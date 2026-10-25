/* FlashVM: value-based frame availability shared by interpreter and generated bodies. */
#include "support.hpp"
#include "frame_loaded_constants.h"
#include "frame_loaded_parse.hpp"
#include "../display_property_ids.h"
#include "../../core/numeric_constants.h"
#include "../../standard/movieclip_constants.h"


/* Convert a direct/value frame request, then query the currently selected native timeline. */
extern "C" FVM_Result
fvm_exec_frame_loaded(FVM_Context *context, FVM_ExecutionFrameCheck policy, const FVM_Value *input, FVM_Value *output)
{
    if (!context || context->destroying || context->gc_releasing || !output ||
        fvm_validate_value(context, input) != FVM_OK ||
        (policy != FVM_EXEC_FRAME_CHECK_DIRECT && policy != FVM_EXEC_FRAME_CHECK_VALUE))
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] String conversion may execute script, collect, throw or select another target. */
        FVM_ExecutionRoots roots(context, FVM_FRAME_LOADED_ROOT_COUNT);
        roots.values[FVM_FRAME_LOADED_INPUT_ROOT] = *input;
        double number = 0;
        FVM_Result status = FVM_OK;
        if (policy == FVM_EXEC_FRAME_CHECK_DIRECT)
        {
            if (input->type != FVM_VALUE_NUMBER || !fvm_frame_integral(input->number) || input->number < 0 ||
                input->number > FVM_MOVIECLIP_MAX_FRAMES)
            {
                return FVM_INVALID_ARGUMENT;
            }
            number = input->number;
        }
        else
        {
            if (input->type == FVM_VALUE_NUMBER && fvm_frame_integral(input->number))
            {
                number = input->number;
            }
            else
            {
                FVM_Value &text = roots.values[FVM_FRAME_LOADED_TEXT_ROOT];
                status = fvm_avm1_string(context, roots.values[FVM_FRAME_LOADED_INPUT_ROOT], &text);
                if (status == FVM_OK)
                {
                    status = fvm_frame_text_number(context, text, &number);
                }
                if (status != FVM_OK)
                {
                    return status;
                }
            }
            number = std::fmod(number, FVM_UINT32_MODULUS);
            if (number < 0)
            {
                number += FVM_UINT32_MODULUS;
            }
            if (number >= FVM_UINT32_SIGN_BIT)
            {
                number -= FVM_UINT32_MODULUS;
            }
            /* Wrapping subtract-one followed by saturating add-one maps Int32 minimum to maximum.
             */
            if (number == FVM_INT32_MIN_NUMBER)
            {
                number = FVM_INT32_MAX_NUMBER;
            }
        }
        long frame = (long)number;
        long limit =
            policy == FVM_EXEC_FRAME_CHECK_DIRECT ? FVM_FRAME_LOADED_DIRECT_LIMIT : FVM_FRAME_LOADED_VALUE_LIMIT;
        if (frame > limit)
        {
            fvm_value_boolean(output, 0);
            return FVM_OK;
        }
        /* Read the target after coercion so scripted target changes remain observable. */
        FVM_Object *clip = context->avm1_target_clip;
        bool available = true;
        if (clip && clip->resource_kind == FVM_RESOURCE_MOVIECLIP)
        {
            FVM_Value loaded, total;
            status = fvm_movieclip_get_action_property(context, clip, FVM_AVM1_PROPERTY_FRAMES_LOADED, &loaded);
            if (status == FVM_OK)
            {
                status = fvm_movieclip_get_action_property(context, clip, FVM_AVM1_PROPERTY_TOTAL_FRAMES, &total);
            }
            if (status != FVM_OK)
            {
                return status;
            }
            double count = loaded.number;
            if (policy == FVM_EXEC_FRAME_CHECK_VALUE)
            {
                frame %= FVM_FRAME_LOADED_U16_MODULUS;
                if (frame < 0)
                {
                    frame += FVM_FRAME_LOADED_U16_MODULUS;
                }
                count += FVM_FRAME_LOADED_VALUE_SLOP;
            }
            double needed = frame < total.number ? (double)frame : total.number;
            available = count >= needed;
        }
        fvm_value_boolean(output, available);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
