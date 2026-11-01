/* Independent signed C division oracle for the private known-bit transfer. */
#include "../optimiser/integer_division_bits.h"
#include <stdio.h>


/* Interpret in-width bits without an out-of-range unsigned-to-signed cast.
 * value and width are copied; return the exact signed32/64 value as int64_t.
 * The sole exceptional magnitude MIN is represented directly, never negated.
 */
static int64_t signed_value(uint64_t value, unsigned width)
{
    uint64_t mask = width == 64 ? UINT64_MAX : UINT32_MAX; /* Active integer bits. */
    uint64_t sign = UINT64_C(1) << (width - 1);            /* Signed representation boundary. */
    uint64_t magnitude = (~value + UINT64_C(1)) & mask;    /* Safe negative magnitude. */
    if (!(value & sign))
    {
        return (int64_t)value;
    }
    if (width == 64 && magnitude == sign)
    {
        return INT64_MIN;
    }
    return -(int64_t)magnitude;
}


/* Enumerate all ternary facts over six low bits, two fixed upper patterns and
 * every sign state. For every positive power-of-two divisor, compare both
 * operations with actual signed C / and %. Unknown signs enumerate both signs.
 * checks is caller-owned and increments per concrete result; return0 on any
 * unsound/noncanonical mask. No signed overflow, VM calls or allocation occur.
 */
static int exercise(unsigned width, unsigned long *checks)
{
    uint64_t mask = width == 64 ? UINT64_MAX : UINT32_MAX; /* Active result mask. */
    uint64_t sign = UINT64_C(1) << (width - 1);            /* Input sign bit. */
    unsigned pattern;                                      /* Fixed upper-pattern selector. */
    unsigned state;                                        /* Base-three low-bit constraint assignment. */
    unsigned signstate;                                    /* Unknown, nonnegative or negative sign constraint. */
    for (pattern = 0; pattern < 2; pattern++)
    {
        for (state = 0; state < 729; state++)
        {
            for (signstate = 0; signstate < 3; signstate++)
            {
                uint64_t forced = (pattern ? UINT64_C(0x3176a8c5e3b29240) : 0) & mask & ~sign;
                uint64_t allow = signstate == 0 ? sign : 0; /* Bits varied by concrete samples. */
                uint64_t zero;                              /* Complement of the admitted sample domain. */
                unsigned digits = state;                    /* Destructive ternary cursor. */
                unsigned bit;                               /* Low-bit position. */
                unsigned shift;                             /* Logarithm of the divisor. */
                if (signstate == 2)
                {
                    forced |= sign;
                }
                for (bit = 0; bit < 6; bit++)
                {
                    if (digits % 3 == 1)
                    {
                        forced |= UINT64_C(1) << bit;
                    }
                    else if (digits % 3 == 2)
                    {
                        allow |= UINT64_C(1) << bit;
                    }
                    digits /= 3;
                }
                zero = mask & ~(forced | allow);
                for (shift = 0; shift < width - 1; shift++)
                {
                    uint64_t divisor = UINT64_C(1) << shift; /* Positive, never sign-bit factor. */
                    int remainder;                           /* Select actual C % or /. */
                    for (remainder = 0; remainder < 2; remainder++)
                    {
                        uint64_t rz = 0; /* Helper's known-zero result. */
                        uint64_t ro = 0; /* Helper's known-one result. */
                        unsigned sample; /* Enumerate all low-bit assignments and both signs. */
                        if (!fir_power_division_bits(zero, forced, divisor, width, remainder, &rz, &ro) || (rz & ro) ||
                            ((rz | ro) & ~mask))
                        {
                            fputs("FAIL division masks\n", stderr);
                            return 0;
                        }
                        for (sample = 0; sample < (signstate == 0 ? 128U : 64U); sample++)
                        {
                            uint64_t value = forced | ((uint64_t)(sample & 63) & allow);
                            int64_t number;   /* Concrete signed input. */
                            int64_t expected; /* C truncates towards zero. */
                            uint64_t actual;  /* Expected in-width result bits. */
                            if (signstate == 0 && (sample & 64))
                            {
                                value |= sign;
                            }
                            number = signed_value(value, width);
                            expected = remainder ? number % (int64_t)divisor : number / (int64_t)divisor;
                            actual = (uint64_t)expected & mask;
                            if ((actual & rz) || (actual & ro) != ro)
                            {
                                fprintf(stderr,
                                        "FAIL signed division width%u state%u sign%u shift%u rem%d sample%u\n",
                                        width,
                                        state,
                                        signstate,
                                        shift,
                                        remainder,
                                        sample);
                                return 0;
                            }
                            (*checks)++;
                        }
                    }
                }
            }
        }
    }
    return 1;
}


/* Require refusal stability and useful precision as well as concrete soundness.
 * No input producer is evaluated: error/effect preservation is checked separately
 * through actual FIR programs. Return nonzero with a diagnostic on failure.
 */
int main(void)
{
    unsigned long checks = 0; /* Concrete signed / and % results checked. */
    uint64_t zero = 17;       /* Refusal canary, later successful result mask. */
    uint64_t one = 29;        /* Refusal canary, later successful result mask. */
    if (!exercise(32, &checks) || !exercise(64, &checks))
    {
        return 1;
    }
    if (fir_power_division_bits(0, 0, 0, 32, 0, &zero, &one) || fir_power_division_bits(0, 0, 3, 64, 0, &zero, &one) ||
        fir_power_division_bits(0, 0, UINT64_C(1) << 31, 32, 0, &zero, &one) ||
        fir_power_division_bits(0, 0, UINT64_C(1) << 63, 64, 0, &zero, &one) ||
        fir_power_division_bits(0, 0, UINT64_C(1) << 40, 32, 0, &zero, &one) ||
        fir_power_division_bits(0, 0, 2, 16, 0, &zero, &one) || zero != 17 || one != 29)
    {
        fputs("FAIL division refusal changed output\n", stderr);
        return 1;
    }
    if (!fir_power_division_bits(UINT32_MAX ^ UINT64_C(255), 0, 8, 32, 0, &zero, &one) ||
        (zero & (UINT32_MAX ^ UINT64_C(31))) != (UINT32_MAX ^ UINT64_C(31)) ||
        !fir_power_division_bits(UINT64_MAX ^ UINT64_C(255), 0, 8, 64, 1, &zero, &one) ||
        (zero & (UINT64_MAX ^ UINT64_C(7))) != (UINT64_MAX ^ UINT64_C(7)))
    {
        fputs("FAIL bounded quotient/remainder precision\n", stderr);
        return 1;
    }
    printf(
        "PASS signed division bits: %lu concrete signed quotient/remainder results, all divisor positions and signs\n",
        checks);
    return 0;
}
