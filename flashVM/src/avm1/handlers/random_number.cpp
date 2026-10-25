/* FlashVM: value-based bounded random numbers for interpreter and generated code. */
#include "support.hpp"
#include "random_number_constants.h"
#include "../../core/random.hpp"


/* Coerce once, saturate to a nonnegative Int32 bound, then draw only for positive bounds. */
extern "C" FVM_Result fvm_exec_random_number(FVM_Context *context, const FVM_Value *maximum, FVM_Value *output)
{
    if (!context || context->destroying || context->gc_releasing || !output)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Value number;
    /* [GC:] Numeric conversion retains maximum through collecting/throwing valueOf callbacks. */
    FVM_Result status = fvm_exec_to_number(context, 0, maximum, &number);
    if (status != FVM_OK)
    {
        return status;
    }
    if (!(number.number >= FVM_EXEC_RANDOM_MINIMUM_BOUND))
    {
        fvm_value_number(output, 0);
        return FVM_OK;
    }
    unsigned long bound =
        number.number >= FVM_INT32_MAX_NUMBER ? (unsigned long)FVM_INT32_MAX_LONG : (unsigned long)number.number;
    fvm_value_number(output, (double)(fvm_random_next(context) % bound));
    return FVM_OK;
}
