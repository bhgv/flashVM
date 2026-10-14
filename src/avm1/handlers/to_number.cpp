/* FlashVM: value-based number and signed integer coercion for interpreter/generated bodies. */
#include "support.hpp"
#include "value_operation_constants.h"
#include "../../core/numeric_constants.h"
/* Convert under loaded-version policy, optionally applying legacy signed Int32 wrapping. */
extern "C" FVM_Result fvm_exec_to_number(FVM_Context *c, int integer, const FVM_Value *value,
                                         FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_EXEC_VALUE_UNARY_ROOT_COUNT);
        roots.values[FVM_EXEC_VALUE_LEFT_ROOT] = *value;
        double number = 0;
        FVM_Result status = fvm_avm1_number(c, &roots.values[FVM_EXEC_VALUE_LEFT_ROOT], &number);
        if (status != FVM_OK)
            return status;
        if (integer)
        {
            if (number != number || number == std::numeric_limits<double>::infinity() ||
                number == -std::numeric_limits<double>::infinity())
                number = 0;
            else
            {
                number = number < 0 ? std::ceil(number) : std::floor(number);
                number = std::fmod(number, FVM_UINT32_MODULUS);
                if (number < 0)
                    number += FVM_UINT32_MODULUS;
                if (number >= FVM_UINT32_SIGN_BIT)
                    number -= FVM_UINT32_MODULUS;
            }
            /* Integer wrapping removes the sign of zero, matching the former bit path. */
            if (number == 0)
                number = 0;
        }
        fvm_value_number(out, number);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
