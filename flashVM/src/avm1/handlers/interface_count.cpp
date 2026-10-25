/* FlashVM: bounded ImplementsOp counts without bytecode or stack dependencies. */
#include "support.hpp"
#include "implements_constants.h"


/* Loaded object counts are zero; primitives use signed Int32 then clamp to available values. */
extern "C" FVM_Result
fvm_exec_interface_count(FVM_Context *c, const FVM_Value *input, size_t available, unsigned int *out)
{
    if (!c || !out || fvm_validate_value(c, input) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (c->swf_loaded && input->type == FVM_VALUE_OBJECT)
    {
        *out = 0;
        return FVM_OK;
    }
    FVM_Value number;
    FVM_Result status =
        fvm_exec_to_number(c, c->swf_loaded ? FVM_IMPLEMENTS_COUNT_INT32 : FVM_IMPLEMENTS_COUNT_NUMBER, input, &number);
    if (status != FVM_OK)
    {
        return status;
    }
    if (!c->swf_loaded)
    {
        if (!(number.number >= 0 && number.number <= FVM_EXEC_MAX_INTERFACE_ARGUMENTS) ||
            number.number != std::floor(number.number) || number.number > available)
        {
            return FVM_INVALID_ARGUMENT;
        }
        *out = (unsigned int)number.number;
        return FVM_OK;
    }
    size_t count = (size_t)(long)number.number;
    if (count > available)
    {
        count = available;
    }
    if (count > FVM_EXEC_MAX_INTERFACE_ARGUMENTS)
    {
        return FVM_ERROR;
    }
    *out = (unsigned int)count;
    return FVM_OK;
}
