/* Private C99 bit transfer shared by arithmetic and sparse products. */
#ifndef FIR_BIT_ARITHMETIC_H
#define FIR_BIT_ARITHMETIC_H
#include <stdint.h>


/* Propagate independent bit facts through modular addition or subtraction.
 * az/ao and bz/bo describe borrowed disjoint known-zero/known-one masks;
 * width is32 or64, subtract selects a+~b+1. Write disjoint result masks to
 * zero/one, owned by the caller. Discard carry beyond width, matching FIR
 * wraparound. Unknown carries remain unknown; correlations are deliberately
 * ignored. No signed arithmetic, allocation, mutation of IR or blocking occurs. */
static void fir_bit_arithmetic(
    uint64_t az, uint64_t ao, uint64_t bz, uint64_t bo, unsigned width, int subtract, uint64_t *zero, uint64_t *one)
{
    unsigned bit;
    unsigned cz = subtract ? 0 : 1;
    unsigned co = subtract ? 1 : 0;
    *zero = 0;
    *one = 0;
    if (subtract)
    {
        uint64_t swap = bz;
        bz = bo;
        bo = swap;
    }
    for (bit = 0; bit < width; bit++)
    {
        uint64_t mask = UINT64_C(1) << bit;
        unsigned a0 = (az & mask) != 0;
        unsigned a1 = (ao & mask) != 0;
        unsigned b0 = (bz & mask) != 0;
        unsigned b1 = (bo & mask) != 0;
        unsigned next_zero = (a0 & b0) | (a0 & cz) | (b0 & cz);
        unsigned next_one = (a1 & b1) | (a1 & co) | (b1 & co);
        if ((a0 & b0 & cz) | (a0 & b1 & co) | (a1 & b0 & co) | (a1 & b1 & cz))
        {
            *zero |= mask;
        }
        if ((a0 & b0 & co) | (a0 & b1 & cz) | (a1 & b0 & cz) | (a1 & b1 & co))
        {
            *one |= mask;
        }
        cz = next_zero;
        co = next_one;
    }
}

#endif
