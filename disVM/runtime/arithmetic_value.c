/* Pure modular arithmetic, separate from admission and guard selection. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "arithmetic_value.h"
#include <stdlib.h>


/* opcode is prevalidated; first/second/output are independently live and jointly
 * guarded. Capture both inputs before any output byte. Unsigned WORD/LONG avoid
 * signed overflow; BYTE promotion fits int. No ownership or callback. */
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
    default:
        abort();
    }
}
