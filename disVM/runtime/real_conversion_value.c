/* Preserve retained FP conversion and rounding policy on owner-local bytes. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "real_conversion_value.h"
#include <stdlib.h>


/* Return widths for the validated native operation without inspecting payload. */
int dvm_runtime_real_conversion_sizes(int opcode, size_t *source, size_t *result)
{
    switch (opcode)
    {
    case ICVTRF:
        *source = sizeof(SREAL);
        *result = sizeof(REAL);
        break;
    case ICVTFR:
        *source = sizeof(REAL);
        *result = sizeof(SREAL);
        break;
    case ICVTWF:
        *source = sizeof(WORD);
        *result = sizeof(REAL);
        break;
    case ICVTLF:
        *source = sizeof(LONG);
        *result = sizeof(REAL);
        break;
    case ICVTFW:
        *source = sizeof(REAL);
        *result = sizeof(WORD);
        break;
    case ICVTFL:
        *source = sizeof(REAL);
        *result = sizeof(LONG);
        break;
    default:
        return 0;
    }
    return 1;
}


/* Borrow only saved bytes; no live guards may surround native FP conversion.
 * Retain half-away-from-zero adjustment and range checks from owning C path. */
const char *dvm_runtime_real_conversion_value(int opcode, const void *source, void *result)
{
    REAL real;
    SREAL small;
    WORD word;
    LONG big;
    switch (opcode)
    {
    case ICVTRF:
        memmove(&small, source, sizeof(small));
        real = small;
        memmove(result, &real, sizeof(real));
        break;
    case ICVTFR:
        memmove(&real, source, sizeof(real));
        small = real;
        memmove(result, &small, sizeof(small));
        break;
    case ICVTWF:
        memmove(&word, source, sizeof(word));
        real = word;
        memmove(result, &real, sizeof(real));
        break;
    case ICVTLF:
        memmove(&big, source, sizeof(big));
        real = big;
        memmove(result, &real, sizeof(real));
        break;
    case ICVTFW:
    case ICVTFL:
        memmove(&real, source, sizeof(real));
        real = real < 0 ? real - .5 : real + .5;
        if (opcode == ICVTFW && sizeof(WORD) == 4)
        {
            if (!(real > -2147483649.0 && real < 2147483648.0))
            {
                return exInval;
            }
        }
        else if (!(real >= -9223372036854775808.0 && real < 9223372036854775808.0))
        {
            return exInval;
        }
        if (opcode == ICVTFW)
        {
            word = (WORD)real;
            memmove(result, &word, sizeof(word));
        }
        else
        {
            big = (LONG)real;
            memmove(result, &big, sizeof(big));
        }
        break;
    default:
        abort();
    }
    return NULL;
}
