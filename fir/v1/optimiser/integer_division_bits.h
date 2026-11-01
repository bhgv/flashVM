/* Private successful-result facts for signed division/remainder by a positive power of two. */
#ifndef FIR_INTEGER_DIVISION_BITS_H
#define FIR_INTEGER_DIVISION_BITS_H
#include "bit_arithmetic.h"


/* Derive one sign-constrained branch from borrowed disjoint az/ao masks.
 * mask covers width bits; shift is below width-1. negative selects the signed
 * branch and remainder selects modulo instead of quotient. Caller owns output
 * zero/one. Unsigned magnitude permits MIN without signed overflow. No IR,
 * producer effects, memory ownership or thread state is changed.
 */
static void fir_division_branch(uint64_t az,
                                uint64_t ao,
                                uint64_t mask,
                                unsigned width,
                                unsigned shift,
                                int negative,
                                int remainder,
                                uint64_t *zero,
                                uint64_t *one)
{
    uint64_t sign = UINT64_C(1) << (width - 1);          /* Constrained input sign bit. */
    uint64_t mz;                                         /* Known-zero unsigned magnitude bits. */
    uint64_t mo;                                         /* Known-one unsigned magnitude bits. */
    uint64_t low = (UINT64_C(1) << shift) - UINT64_C(1); /* Remainder magnitude mask. */
    if (negative)
    {
        fir_bit_arithmetic(mask, 0, az, ao | sign, width, 1, &mz, &mo);
    }
    else
    {
        mz = az | sign;
        mo = ao;
    }
    if (remainder)
    {
        mz |= mask ^ low;
        mo &= low;
    }
    else
    {
        mz = (mz >> shift) | (mask ^ (mask >> shift));
        mo >>= shift;
    }
    if (negative)
    {
        fir_bit_arithmetic(mask, 0, mz, mo, width, 1, zero, one);
    }
    else
    {
        *zero = mz;
        *one = mo;
    }
}


/* Analyze a signed quotient/remainder with exact positive power-of-two divisor.
 * az/ao are borrowed disjoint in-width masks, width must be32/64; remainder
 * selects truncating modulo. Return0 for unsupported width/divisor and leave
 * both disjoint non-null output cells unchanged. Return1 with sound masks on
 * success. Unknown signs are split and joined by intersection. This is bounded
 * analysis only: no divide instruction or eager error-producing operand is
 * removed, no callbacks/allocation/blocking or signed overflow occurs.
 */
static int fir_power_division_bits(
    uint64_t az, uint64_t ao, uint64_t divisor, unsigned width, int remainder, uint64_t *zero, uint64_t *one)
{
    uint64_t mask;           /* Active two's-complement width. */
    uint64_t sign;           /* Excluded negative divisor bit. */
    uint64_t pz = 0;         /* Nonnegative branch known-zero result. */
    uint64_t po = 0;         /* Nonnegative branch known-one result. */
    uint64_t nz = 0;         /* Negative branch known-zero result. */
    uint64_t no = 0;         /* Negative branch known-one result. */
    unsigned shift = 0;      /* Exact logarithm of the accepted divisor. */
    uint64_t bits = divisor; /* Unsigned logarithm cursor. */
    if (width != 32 && width != 64)
    {
        return 0;
    }
    mask = width == 64 ? UINT64_MAX : UINT32_MAX;
    sign = UINT64_C(1) << (width - 1);
    if (!divisor || (divisor & (divisor - UINT64_C(1))) || (divisor & ~mask) || (divisor & sign))
    {
        return 0;
    }
    while (bits > 1)
    {
        bits >>= 1;
        shift++;
    }
    if (!(ao & sign))
    {
        fir_division_branch(az, ao, mask, width, shift, 0, remainder, &pz, &po);
    }
    if (!(az & sign))
    {
        fir_division_branch(az, ao, mask, width, shift, 1, remainder, &nz, &no);
    }
    if (az & sign)
    {
        *zero = pz;
        *one = po;
    }
    else if (ao & sign)
    {
        *zero = nz;
        *one = no;
    }
    else
    {
        *zero = pz & nz;
        *one = po & no;
    }
    return 1;
}
#endif
