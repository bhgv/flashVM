/* FlashVM: StringEquals/StringLess/StringGreater with their historical coercion order. */
#include "support.hpp"
#include "string_operation_constants.h"
#include "../../standard/string_order.hpp"


/* Compare explicit strings by UTF-16 units; greater converts left first, other modes right first.
 */
extern "C" FVM_Result fvm_exec_string_compare(
    FVM_Context *c, FVM_ExecutionCompare operation, const FVM_Value *left, const FVM_Value *right, FVM_Value *out)
{
    if (!c || !out || (operation != FVM_EXEC_EQUAL && operation != FVM_EXEC_LESS && operation != FVM_EXEC_GREATER) ||
        fvm_validate_value(c, left) != FVM_OK || fvm_validate_value(c, right) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Retain both input graphs and each converted result across nested script/GC. */
        FVM_ExecutionRoots roots(c, FVM_STRING_BINARY_ROOT_COUNT);
        roots.values[FVM_STRING_BINARY_LEFT_ROOT] = *left;
        roots.values[FVM_STRING_BINARY_RIGHT_ROOT] = *right;
        FVM_Value &a = roots.values[FVM_STRING_BINARY_LEFT_TEXT_ROOT];
        FVM_Value &b = roots.values[FVM_STRING_BINARY_RIGHT_TEXT_ROOT];
        FVM_Result status;
        if (operation == FVM_EXEC_GREATER)
        {
            status = fvm_avm1_string(c, roots.values[FVM_STRING_BINARY_LEFT_ROOT], &a);
            if (status == FVM_OK)
            {
                status = fvm_avm1_string(c, roots.values[FVM_STRING_BINARY_RIGHT_ROOT], &b);
            }
        }
        else
        {
            status = fvm_avm1_string(c, roots.values[FVM_STRING_BINARY_RIGHT_ROOT], &b);
            if (status == FVM_OK)
            {
                status = fvm_avm1_string(c, roots.values[FVM_STRING_BINARY_LEFT_ROOT], &a);
            }
        }
        if (status != FVM_OK)
        {
            return status;
        }
        int order = 0;
        status = fvm_string_order(a, b, order);
        if (status == FVM_OK)
        {
            fvm_value_boolean(out,
                              operation == FVM_EXEC_EQUAL  ? order == 0
                              : operation == FVM_EXEC_LESS ? order < 0
                                                           : order > 0);
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
