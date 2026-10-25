/* FlashVM: value-based frame/label preparation shared by C clients and generated bodies. */
#include "support.hpp"
#include "flashvm/standard.h"
#include "frame_request_constants.h"


/* Convert nonnumbers once, recognize complete decimal digits, and bias only numeric requests. */
extern "C" FVM_Result fvm_exec_frame_request(
    FVM_Context *context, const FVM_Value *input, int with_bias, unsigned long scene_bias, FVM_Value *output)
{
    if (!context || context->destroying || context->gc_releasing || !output ||
        scene_bias > FVM_EXEC_MAX_SCENE_FRAME_BIAS || fvm_validate_value(context, input) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Conversion may run script, collect, throw or change the selected target. */
        FVM_ExecutionRoots roots(context, FVM_FRAME_REQUEST_ROOT_COUNT);
        roots.values[FVM_FRAME_REQUEST_INPUT_ROOT] = *input;
        FVM_Value &result = roots.values[FVM_FRAME_REQUEST_TEXT_ROOT];
        FVM_Result status = FVM_OK;
        if (input->type == FVM_VALUE_NUMBER)
        {
            fvm_value_number(&result, input->number);
        }
        else
        {
            status = fvm_avm1_string(context, roots.values[FVM_FRAME_REQUEST_INPUT_ROOT], &result);
        }
        if (status != FVM_OK)
        {
            return status;
        }
        if (result.type == FVM_VALUE_STRING)
        {
            size_t size = fvm_string_size(result);
            bool decimal = size != 0;
            for (size_t at = 0; at < size; ++at)
            {
                if (result.string[at] < '0' || result.string[at] > '9')
                {
                    decimal = false;
                }
            }
            if (decimal)
            {
                double number = 0;
                status = fvm_value_to_number(context, &result, &number);
                if (status != FVM_OK)
                {
                    return status;
                }
                fvm_value_number(&result, number);
            }
        }
        if (with_bias && result.type == FVM_VALUE_NUMBER)
        {
            result.number += scene_bias;
        }
        *output = result;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
