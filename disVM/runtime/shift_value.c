/* Pure bounded integer shifts; signed right shift is defined through bits. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "shift_value.h"
#include <stdlib.h>


/* opcode selects the value width; second/count are borrowed guarded/private
 * storage respectively. Return1 for a legal captured WORD count,0 otherwise.
 * No value/result access, C shift expression or throwing callback occurs. */
int dvm_runtime_shift_count(int opcode, const void *second, WORD *count)
{
    WORD bits;
    switch (opcode)
    {
    case ISHLB:
    case ISHRB:
        bits = 8;
        break;
    case ISHLW:
    case ISHRW:
    case ILSRW:
        bits = sizeof(WORD)*8;
        break;
    case ISHLL:
    case ISHRL:
    case ILSRL:
        bits = sizeof(LONG)*8;
        break;
    default:
        abort();
    }
    memmove(count, second, sizeof(*count));
    return *count >= 0 && *count < bits;
}


/* opcode/first/count/output follow the header's preconditions. Capture all value
 * bits before the possibly overlapping store. Unsigned shifts avoid signed-left
 * overflow; sign fill preserves arithmetic right shift without a signed C shift.
 * Return void, no reference ownership, FP, callback or allocation. */
void dvm_runtime_shift_value(int opcode, const void *first, WORD count, void *output)
{
    switch (opcode)
    {
    case ISHLB:
    {
        BYTE value;
        BYTE result;
        memmove(&value, first, sizeof(value));
        result = value << count;
        memmove(output, &result, sizeof(result));
        break;
    }
    case ISHLW:
    {
        UWORD value;
        UWORD result;
        memmove(&value, first, sizeof(value));
        result = value << count;
        memmove(output, &result, sizeof(result));
        break;
    }
    case ISHLL:
    {
        ULONG value;
        ULONG result;
        memmove(&value, first, sizeof(value));
        result = value << count;
        memmove(output, &result, sizeof(result));
        break;
    }
    case ISHRB:
    {
        BYTE value;
        BYTE result;
        memmove(&value, first, sizeof(value));
        result = value >> count;
        memmove(output, &result, sizeof(result));
        break;
    }
    case ISHRW:
    {
        UWORD value;
        UWORD result;
        memmove(&value, first, sizeof(value));
        result = value >> count;
        if (count && (value & ((UWORD)1 << (sizeof(value)*8-1))))
        {
            result |= ~(UWORD)0 << (sizeof(value)*8-count);
        }
        memmove(output, &result, sizeof(result));
        break;
    }
    case ISHRL:
    {
        ULONG value;
        ULONG result;
        memmove(&value, first, sizeof(value));
        result = value >> count;
        if (count && (value & ((ULONG)1 << (sizeof(value)*8-1))))
        {
            result |= ~(ULONG)0 << (sizeof(value)*8-count);
        }
        memmove(output, &result, sizeof(result));
        break;
    }
    case ILSRW:
    {
        UWORD value;
        UWORD result;
        memmove(&value, first, sizeof(value));
        result = value >> count;
        memmove(output, &result, sizeof(result));
        break;
    }
    case ILSRL:
    {
        ULONG value;
        ULONG result;
        memmove(&value, first, sizeof(value));
        result = value >> count;
        memmove(output, &result, sizeof(result));
        break;
    }
    default:
        abort();
    }
}
