/* FlashVM: legacy numeric equality/ordering with historical conversion order. */
#include "support.hpp"
#include "value_operation_constants.h"
/* Project a metamethod result with the existing value truth rules. */
static bool hook_truth(const FVM_Value &value)
{
    switch (value.type)
    {
    case FVM_VALUE_BOOLEAN:
        return value.boolean != 0;
    case FVM_VALUE_NUMBER:
        return value.number != 0 && value.number == value.number;
    case FVM_VALUE_STRING:
        return value.string && fvm_string_size(value) != 0;
    case FVM_VALUE_OBJECT:
        return true;
    default:
        return false;
    }
}
/* Compare retained values without accepting bytecode or an interpreter stack. */
extern "C" FVM_Result fvm_exec_legacy_compare(FVM_Context *c, FVM_ExecutionLegacyCompare operation,
                                              const FVM_Value *left, const FVM_Value *right,
                                              FVM_Value *out)
{
    if (!c || !out || (operation != FVM_EXEC_LEGACY_EQUAL && operation != FVM_EXEC_LEGACY_LESS) ||
        fvm_validate_value(c, left) != FVM_OK || fvm_validate_value(c, right) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_EXEC_VALUE_BINARY_ROOT_COUNT);
        roots.values[FVM_EXEC_VALUE_LEFT_ROOT] = *left;
        roots.values[FVM_EXEC_VALUE_RIGHT_ROOT] = *right;
        const FVM_Value *operands = roots.values.begin();
        FVM_Value result;
        const char *hook = operation == FVM_EXEC_LEGACY_EQUAL ? "__eq" : "__lt";
        FVM_Result status = fvm_value_meta_invoke(
            c, operands, hook, FVM_EXEC_VALUE_BINARY_ROOT_COUNT, operands, &result);
        if (status == FVM_NOT_FOUND)
            status = fvm_value_meta_invoke(c, operands + FVM_EXEC_VALUE_RIGHT_ROOT, hook,
                                           FVM_EXEC_VALUE_BINARY_ROOT_COUNT, operands, &result);
        bool matches = false;
        if (status == FVM_OK)
            matches = hook_truth(result);
        else if (status != FVM_NOT_FOUND)
            return status;
        else
        {
            double x = 0, y = 0;
            if (operation == FVM_EXEC_LEGACY_EQUAL)
            {
                status = fvm_avm1_number(c, operands + FVM_EXEC_VALUE_RIGHT_ROOT, &y);
                if (status == FVM_OK)
                    status = fvm_avm1_number(c, operands, &x);
            }
            else
            {
                status = fvm_avm1_number(c, operands, &x);
                if (status == FVM_OK)
                    status = fvm_avm1_number(c, operands + FVM_EXEC_VALUE_RIGHT_ROOT, &y);
            }
            if (status != FVM_OK)
                return status;
            matches = operation == FVM_EXEC_LEGACY_EQUAL ? x == y : x < y;
        }
        fvm_value_boolean(out, matches);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
