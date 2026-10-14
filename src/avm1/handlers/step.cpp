/* FlashVM: value-based numeric stepping shared by interpreted and generated bodies. */
#include "flashvm/execution.h"
#include "step_constants.h"
/* Coerce once under the current SWF policy, then publish a numeric result; aliases are allowed. */
extern "C" FVM_Result fvm_exec_step(FVM_Context *context, int decrement, const FVM_Value *input,
                                    FVM_Value *output)
{
    if (!output)
        return FVM_INVALID_ARGUMENT;
    FVM_Value converted;
    /* [GC:] The common number service roots input through valueOf, collection and exceptions. */
    FVM_Result status = fvm_exec_to_number(context, 0, input, &converted);
    if (status != FVM_OK)
        return status;
    fvm_value_number(output,
                     converted.number + (decrement ? -FVM_EXEC_STEP_UNIT : FVM_EXEC_STEP_UNIT));
    return FVM_OK;
}
