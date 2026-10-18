/* FlashVM: version-aware string coercion for C callers and generated bodies. */
#include "support.hpp"
#include "value_operation_constants.h"
/* Retain a value while its conversion callback may collect or raise an exception. */
extern "C" FVM_Result fvm_exec_to_string(FVM_Context *c, const FVM_Value *value, FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_EXEC_VALUE_UNARY_ROOT_COUNT);
        roots.values[FVM_EXEC_VALUE_LEFT_ROOT] = *value;
        FVM_Value result;
        FVM_Result status = fvm_avm1_string(c, roots.values[FVM_EXEC_VALUE_LEFT_ROOT], &result);
        if (status == FVM_OK)
            *out = result;
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
