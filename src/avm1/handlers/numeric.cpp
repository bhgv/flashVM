/* FlashVM: numeric semantics for C callers and generated code. */
#include "support.hpp"
/* Convert a numeric operand to the low 32 unsigned bits without undefined float-to-int casts. */
static unsigned long integer_bits(double value)
{
    if (value != value || value == std::numeric_limits<double>::infinity() ||
        value == -std::numeric_limits<double>::infinity())
        return 0;
    value = value < 0 ? std::ceil(value) : std::floor(value);
    value = std::fmod(value, 4294967296.0);
    return (unsigned long)(value < 0 ? value + 4294967296.0 : value);
}
/* Represent a low 32-bit result as a signed ActionScript Number on both 32/64-bit hosts. */
static double signed_bits(unsigned long value)
{
    value &= 0xffffffffUL;
    return value >= 0x80000000UL ? (double)value - 4294967296.0 : (double)value;
}
/* Execute value semantics with validation and allocation failure containment. */
extern "C" FVM_Result fvm_exec_numeric(FVM_Context *c, FVM_ExecutionNumeric operation,
                                       const FVM_Value *left_value, const FVM_Value *right_value,
                                       FVM_Value *out)
{
    if (!c || !out || operation < FVM_EXEC_ADD || operation > FVM_EXEC_SHIFT_UNSIGNED ||
        fvm_validate_value(c, left_value) != FVM_OK || fvm_validate_value(c, right_value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, 2);
        roots.values[0] = *left_value;
        roots.values[1] = *right_value;
        if (operation <= FVM_EXEC_DIVIDE)
        {
            /* [GC:] Both operands stay in the temporary roots across hooks and coercion. */
            FVM_Value *operands = roots.values.begin();
            FVM_Value result;
            const char *hook = operation == FVM_EXEC_ADD        ? "__add"
                               : operation == FVM_EXEC_SUBTRACT ? "__sub"
                               : operation == FVM_EXEC_MULTIPLY ? "__mul"
                                                                : "__div";
            FVM_Result status = fvm_value_meta_invoke(c, operands, hook, 2, operands, &result);
            if (status == FVM_NOT_FOUND)
                status = fvm_value_meta_invoke(c, operands + 1, hook, 2, operands, &result);
            if (status != FVM_OK && status != FVM_NOT_FOUND)
                return status;
            if (status == FVM_NOT_FOUND)
            {
                double left = 0, right = 0;
                /* Subtract/Divide convert right first; Add/Multiply convert left first. */
                if (operation == FVM_EXEC_SUBTRACT || operation == FVM_EXEC_DIVIDE)
                {
                    status = fvm_avm1_number(c, operands + 1, &right);
                    if (status == FVM_OK)
                        status = fvm_avm1_number(c, operands, &left);
                }
                else
                {
                    status = fvm_avm1_number(c, operands, &left);
                    if (status == FVM_OK)
                        status = fvm_avm1_number(c, operands + 1, &right);
                }
                if (status != FVM_OK)
                    return status;
                if (operation == FVM_EXEC_DIVIDE && right == 0 && c->swf_loaded &&
                    c->swf_info.version < 5)
                    status = fvm_value_string(&result, "#ERROR#");
                else
                    fvm_value_number(&result, operation == FVM_EXEC_ADD        ? left + right
                                              : operation == FVM_EXEC_SUBTRACT ? left - right
                                              : operation == FVM_EXEC_MULTIPLY ? left * right
                                                                               : left / right);
                if (status != FVM_OK)
                    return status;
            }
            *out = result;
            return FVM_OK;
        }
        /* Convert operands in the historical remainder/bit-operation order. */
        double left = 0, right = 0;
        /* Remainder and bit operations convert the top/right operand first. */
        FVM_Result r = fvm_avm1_number(c, &roots.values[1], &right);
        if (r == FVM_OK)
            r = fvm_avm1_number(c, &roots.values[0], &left);
        if (r != FVM_OK)
            return r;
        double result = 0;
        if (operation == FVM_EXEC_REMAINDER)
        {
            if (!right || left != left || right != right ||
                std::fabs(left) == std::numeric_limits<double>::infinity())
                result = std::numeric_limits<double>::quiet_NaN();
            else if (std::fabs(right) == std::numeric_limits<double>::infinity())
                result = left;
            else
                result = std::fmod(left, right);
        }
        else
        {
            unsigned long value = integer_bits(left), operand = integer_bits(right);
            unsigned int shift = (unsigned int)(operand & 31);
            unsigned long bits = 0;
            if (operation == FVM_EXEC_BIT_AND)
                bits = value & operand;
            else if (operation == FVM_EXEC_BIT_OR)
                bits = value | operand;
            else if (operation == FVM_EXEC_BIT_XOR)
                bits = value ^ operand;
            else if (operation == FVM_EXEC_SHIFT_LEFT)
                bits = (value << shift) & 0xffffffffUL;
            else
            {
                bits = value >> shift;
                /* Arithmetic right shift is implemented with unsigned masks, avoiding
                 * implementation-defined signed shifts and shifts by the host word width.
                 */
                if (operation == FVM_EXEC_SHIFT_RIGHT && shift && (value & 0x80000000UL))
                    bits |= (0xffffffffUL << (32 - shift)) & 0xffffffffUL;
            }
            bool unsigned_result =
                operation == FVM_EXEC_SHIFT_UNSIGNED && (!c->swf_loaded || c->swf_info.version < 8);
            result = unsigned_result ? (double)bits : signed_bits(bits);
        }
        fvm_value_number(out, result);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
