/* FlashVM: version-aware logical value operations for C and generated code. */
#include "support.hpp"
#include "value_operation_constants.h"
/* Convert left then right without short-circuiting; unary NOT ignores right. */
extern "C" FVM_Result fvm_exec_logical(FVM_Context *c, FVM_ExecutionLogical operation,
                                       const FVM_Value *left, const FVM_Value *right,
                                       FVM_Value *out)
{
    if (!c || !out || operation < FVM_EXEC_LOGICAL_NOT || operation > FVM_EXEC_LOGICAL_OR ||
        fvm_validate_value(c, left) != FVM_OK ||
        (operation != FVM_EXEC_LOGICAL_NOT && fvm_validate_value(c, right) != FVM_OK))
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_EXEC_VALUE_BINARY_ROOT_COUNT);
        roots.values[FVM_EXEC_VALUE_LEFT_ROOT] = *left;
        if (operation != FVM_EXEC_LOGICAL_NOT)
            roots.values[FVM_EXEC_VALUE_RIGHT_ROOT] = *right;
        bool x = false, y = false;
        FVM_Result status = fvm_avm1_boolean(c, roots.values[FVM_EXEC_VALUE_LEFT_ROOT], &x);
        if (status == FVM_OK && operation != FVM_EXEC_LOGICAL_NOT)
            status = fvm_avm1_boolean(c, roots.values[FVM_EXEC_VALUE_RIGHT_ROOT], &y);
        if (status != FVM_OK)
            return status;
        fvm_value_boolean(out, operation == FVM_EXEC_LOGICAL_NOT   ? !x
                               : operation == FVM_EXEC_LOGICAL_AND ? x && y
                                                                   : x || y);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
