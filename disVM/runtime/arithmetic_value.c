/* Pure integer arithmetic, separate from admission and guard selection. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "arithmetic_value.h"
#include <stdlib.h>


/* opcode is prevalidated; first/second/output are independently live and jointly
 * guarded. Capture both inputs before any output byte. Modular operations use
 * unsigned WORD/LONG; signed DIV/MOD require nonzero and handle MIN/-1.
 * BYTE promotion fits int. No ownership or callback. */
void dvm_runtime_arithmetic_value(int opcode, const void *first, const void *second, void *output)
{
    switch (opcode)
    {
    case IADDB:
    {
        BYTE left;
        BYTE right;
        BYTE result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left + right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IADDW:
    {
        UWORD left;
        UWORD right;
        UWORD result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left + right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IADDL:
    {
        ULONG left;
        ULONG right;
        ULONG result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left + right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case ISUBB:
    {
        BYTE left;
        BYTE right;
        BYTE result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left - right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case ISUBW:
    {
        UWORD left;
        UWORD right;
        UWORD result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left - right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case ISUBL:
    {
        ULONG left;
        ULONG right;
        ULONG result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left - right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IANDB:
    {
        BYTE left;
        BYTE right;
        BYTE result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left & right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IANDW:
    {
        UWORD left;
        UWORD right;
        UWORD result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left & right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IANDL:
    {
        ULONG left;
        ULONG right;
        ULONG result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left & right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IORB:
    {
        BYTE left;
        BYTE right;
        BYTE result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left | right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IORW:
    {
        UWORD left;
        UWORD right;
        UWORD result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left | right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IORL:
    {
        ULONG left;
        ULONG right;
        ULONG result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left | right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IXORB:
    {
        BYTE left;
        BYTE right;
        BYTE result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left ^ right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IXORW:
    {
        UWORD left;
        UWORD right;
        UWORD result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left ^ right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IXORL:
    {
        ULONG left;
        ULONG right;
        ULONG result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left ^ right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IMULB:
    {
        BYTE left;
        BYTE right;
        BYTE result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left * right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IMULW:
    {
        UWORD left;
        UWORD right;
        UWORD result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left * right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IMULL:
    {
        ULONG left;
        ULONG right;
        ULONG result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left * right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IDIVB:
    {
        BYTE left;
        BYTE right;
        BYTE result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left / right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IDIVW:
    {
        WORD left;
        WORD right;
        WORD result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        if (right == -1 && (UWORD)left == ((UWORD)1 << (sizeof(WORD)*8-1)))
        {
            result = left;
        }
        else
        {
            result = left / right;
        }
        memmove(output, &result, sizeof(result));
        break;
    }
    case IDIVL:
    {
        LONG left;
        LONG right;
        LONG result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        if (right == -1 && (ULONG)left == ((ULONG)1 << (sizeof(LONG)*8-1)))
        {
            result = left;
        }
        else
        {
            result = left / right;
        }
        memmove(output, &result, sizeof(result));
        break;
    }
    case IMODB:
    {
        BYTE left;
        BYTE right;
        BYTE result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        result = left % right;
        memmove(output, &result, sizeof(result));
        break;
    }
    case IMODW:
    {
        WORD left;
        WORD right;
        WORD result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        if (right == -1 && (UWORD)left == ((UWORD)1 << (sizeof(WORD)*8-1)))
        {
            result = 0;
        }
        else
        {
            result = left % right;
        }
        memmove(output, &result, sizeof(result));
        break;
    }
    case IMODL:
    {
        LONG left;
        LONG right;
        LONG result;
        memmove(&left, first, sizeof(left));
        memmove(&right, second, sizeof(right));
        if (right == -1 && (ULONG)left == ((ULONG)1 << (sizeof(LONG)*8-1)))
        {
            result = 0;
        }
        else
        {
            result = left % right;
        }
        memmove(output, &result, sizeof(result));
        break;
    }
    default:
        abort();
    }
}


/* [PERF MULDIV-098] Nonzero-divisor precheck participates in the whole-profile
 * comparison, not a separate timing of this function. */


/* opcode is one of six DIV/MOD variants; second is a valid jointly guarded cell.
 * Return1 for zero divisor,0 otherwise. No other operand, callback or FP effect.
 * Unknown internal opcode aborts; caller validates before guard acquisition. */
int dvm_runtime_divisor_zero(int opcode, const void *second)
{
    switch (opcode)
    {
    case IDIVB:
    case IMODB:
    {
        BYTE value;
        memmove(&value, second, sizeof(value));
        return value == 0;
    }
    case IDIVW:
    case IMODW:
    {
        WORD value;
        memmove(&value, second, sizeof(value));
        return value == 0;
    }
    case IDIVL:
    case IMODL:
    {
        LONG value;
        memmove(&value, second, sizeof(value));
        return value == 0;
    }
    default:
        abort();
    }
}
