/* Private bounded transfer for exact one/two-bit modular factors. */
#ifndef FIR_INTEGER_PRODUCT_BITS_H
#define FIR_INTEGER_PRODUCT_BITS_H
#include "bit_arithmetic.h"


/* Count low zeroes of a nonzero unsigned factor already truncated to width.
 * Return0..63; value is copied, no signed shift, allocation or callbacks occur. */
static unsigned fir_product_shift(uint64_t value)
{
    unsigned shift = 0;
    while (!(value & UINT64_C(1)))
    {
        value >>= 1;
        shift++;
    }
    return shift;
}


/* Derive a*(2^lo [+2^hi]) modulo2^width from disjoint borrowed az/ao masks.
 * width must be32/64; factor must be an exact in-width nonzero constant with
 * at most two set bits. On unsupported factor/width return0 without changing
 * caller-owned zero/one. Outputs must be disjoint non-null pointers. Otherwise
 * return1 and write disjoint sound facts.
 * Addition ignores correlations between shifted copies: reduced precision is
 * safe; it cannot invent a carry fact. No producer is executed or removed. */
static int
fir_sparse_product_bits(uint64_t az, uint64_t ao, uint64_t factor, unsigned width, uint64_t *zero, uint64_t *one)
{
    uint64_t mask;
    uint64_t low;
    uint64_t rest;
    uint64_t azero;
    uint64_t aone;
    unsigned shift;
    if (width != 32 && width != 64)
    {
        return 0;
    }
    mask = width == 64 ? UINT64_MAX : UINT32_MAX;
    if (!factor || (factor & mask) != factor)
    {
        return 0;
    }
    low = factor & (~factor + UINT64_C(1));
    rest = factor ^ low;
    if (rest && (rest & (rest - UINT64_C(1))))
    {
        return 0;
    }
    shift = fir_product_shift(low);
    azero = ((az << shift) | ((UINT64_C(1) << shift) - UINT64_C(1))) & mask;
    aone = (ao << shift) & mask;
    if (rest)
    {
        uint64_t bzero;
        uint64_t bone;
        shift = fir_product_shift(rest);
        bzero = ((az << shift) | ((UINT64_C(1) << shift) - UINT64_C(1))) & mask;
        bone = (ao << shift) & mask;
        fir_bit_arithmetic(azero, aone, bzero, bone, width, 0, zero, one);
    }
    else
    {
        *zero = azero;
        *one = aone;
    }
    return 1;
}
#endif
