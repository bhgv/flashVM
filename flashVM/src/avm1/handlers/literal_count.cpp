/* FlashVM: versioned Array/Object literal count conversion independent of encoded operands. */
#include "support.hpp"
#include "literal_constants.h"
#include "../../core/numeric_constants.h"


/* Return a normalized Number or the loaded invalid-count sentinel without constructing objects. */
extern "C" FVM_Result fvm_exec_literal_count(FVM_Context *c, const FVM_Value *input, FVM_Value *out)
{
    if (!c || c->destroying || c->gc_releasing || !out || fvm_validate_value(c, input) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {
        double count = 0;
        if (c->swf_loaded)
        {


            /* [GC:] The original coercible object may have no other owner in a C/JIT caller. */
            FVM_ExecutionRoots roots(c, FVM_LITERAL_COUNT_ROOT_COUNT);
            roots.values[FVM_LITERAL_COUNT_INPUT_ROOT] = *input;
            FVM_Result status = fvm_avm1_number(c, &roots.values[FVM_LITERAL_COUNT_INPUT_ROOT], &count);
            if (status != FVM_OK)
            {
                return status;
            }
            if (count < 0 || count > FVM_INT32_MAX_NUMBER)
            {
                fvm_value_undefined(out);
                return FVM_OK;
            }
            count = count == count ? std::floor(count) : 0;
            if (count > FVM_EXEC_MAX_LITERAL_ENTRIES)
            {
                return FVM_UNSUPPORTED;
            }
        }
        else
        {
            if (input->type != FVM_VALUE_NUMBER)
            {
                return FVM_ERROR;
            }
            count = input->number;
            if (!(count >= 0 && count <= FVM_EXEC_MAX_LITERAL_ENTRIES) || count != std::floor(count))
            {
                return FVM_INVALID_ARGUMENT;
            }
        }
        fvm_value_number(out, count);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
