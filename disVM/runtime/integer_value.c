/* Integer conversion mechanics, independent of admission and locking. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "integer_value.h"
#include <stdlib.h>


/* opcode selects one of six prevalidated integer assignments. source and
 * destination are live byte spans, already validated and jointly guarded.
 * Preserve the retained C integer assignment semantics at native WORD width.
 * Capture the complete input before any output byte; no managed references. */
void dvm_runtime_integer_value(int opcode, const void *source, void *destination)
{
    switch (opcode)
    {
    case ICVTBW:
    {
        BYTE input;
        WORD output;
        memmove(&input, source, sizeof(input));
        output = input;
        memmove(destination, &output, sizeof(output));
        break;
    }
    case ICVTWB:
    {
        WORD input;
        BYTE output;
        memmove(&input, source, sizeof(input));
        output = input;
        memmove(destination, &output, sizeof(output));
        break;
    }
    case ICVTWS:
    {
        WORD input;
        SHORT output;
        memmove(&input, source, sizeof(input));
        output = input;
        memmove(destination, &output, sizeof(output));
        break;
    }
    case ICVTSW:
    {
        SHORT input;
        WORD output;
        memmove(&input, source, sizeof(input));
        output = input;
        memmove(destination, &output, sizeof(output));
        break;
    }
    case ICVTWL:
    {
        WORD input;
        LONG output;
        memmove(&input, source, sizeof(input));
        output = input;
        memmove(destination, &output, sizeof(output));
        break;
    }
    case ICVTLW:
    {
        LONG input;
        WORD output;
        memmove(&input, source, sizeof(input));
        output = input;
        memmove(destination, &output, sizeof(output));
        break;
    }
    default:
        abort();
    }
}
