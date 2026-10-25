/* FlashVM: bounded call-count conversion independent of bytecode and operand layout. */
#include "support.hpp"
#include "value_operation_constants.h"
#include "../../core/numeric_constants.h"


/* Convert loaded AVM1 counts to unsigned 32 bits and consume at most available values. */
extern "C" FVM_Result
fvm_exec_argument_count(FVM_Context *c, const FVM_Value *value, size_t available, unsigned int *out)
{
    if (!c || !out || fvm_validate_value(c, value) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {
        if (!c->swf_loaded)
        {
            if (value->type != FVM_VALUE_NUMBER ||
                !(value->number >= 0 && value->number <= FVM_EXEC_MAX_METHOD_ARGUMENTS) ||
                value->number != std::floor(value->number) || value->number > available)
            {
                return FVM_ERROR;
            }
            *out = (unsigned int)value->number;
            return FVM_OK;
        }


        FVM_ExecutionRoots roots(c, FVM_EXEC_VALUE_UNARY_ROOT_COUNT);
        roots.values[FVM_EXEC_VALUE_LEFT_ROOT] = *value;
        double count = 0;
        FVM_Result status = fvm_avm1_number(c, &roots.values[FVM_EXEC_VALUE_LEFT_ROOT], &count);
        if (status != FVM_OK)
        {
            return status;
        }
        if (count != count || count == std::numeric_limits<double>::infinity() ||
            count == -std::numeric_limits<double>::infinity())
        {
            count = 0;
        }
        else
        {
            count = count < 0 ? std::ceil(count) : std::floor(count);
            count = std::fmod(count, FVM_UINT32_MODULUS);
            if (count < 0)
            {
                count += FVM_UINT32_MODULUS;
            }
        }
        size_t actual = (size_t)(unsigned long)count;
        if (actual > available)
        {
            actual = available;
        }
        if (actual > FVM_EXEC_MAX_METHOD_ARGUMENTS)
        {
            return FVM_ERROR;
        }
        *out = (unsigned int)actual;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
