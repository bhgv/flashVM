/* Exhaustive low-byte values and every one/two-bit factor: independent unsigned-product oracle. */
#include "../optimiser/integer_product_bits.h"
#include <stdio.h>


/* Check all admitted factors for width32/64 against concrete modular products.
 * Unknown bits occupy a small enumerable byte; forced high bits include signs.
 * Return0 on a contradictory/unsound fact,1 otherwise. No VM or allocation. */
static int exercise(unsigned width, unsigned long *checks)
{
    const uint64_t allow[] = {255, 85, 129, 3, 0};
    uint64_t mask = width == 64 ? UINT64_MAX : UINT32_MAX;
    unsigned lo;
    unsigned hi;
    unsigned pattern;
    for (lo = 0; lo < width; lo++)
    {
        for (hi = lo; hi < width; hi++)
        {
            uint64_t factor = (UINT64_C(1) << lo) | (UINT64_C(1) << hi);
            for (pattern = 0; pattern < sizeof(allow) / sizeof(allow[0]); pattern++)
            {
                uint64_t forced = UINT64_C(0x9137d3ab8c5ef029) & mask & ~allow[pattern];
                uint64_t zero;
                uint64_t one;
                unsigned sample;
                if (!fir_sparse_product_bits(mask & ~(allow[pattern] | forced), forced, factor, width, &zero, &one) ||
                    (zero & one) || ((zero | one) & ~mask))
                {
                    fputs("FAIL sparse product: invalid fact masks\n", stderr);
                    return 0;
                }
                for (sample = 0; sample < 256; sample++)
                {
                    uint64_t value = (sample & allow[pattern]) | forced;
                    uint64_t actual = (value * factor) & mask;
                    if ((actual & zero) || (actual & one) != one)
                    {
                        fprintf(stderr,
                                "FAIL sparse product: width%u factor bits%u/%u pattern%u sample%u\n",
                                width,
                                lo,
                                hi,
                                pattern,
                                sample);
                        return 0;
                    }
                    (*checks)++;
                }
            }
        }
    }
    return 1;
}


/* Run both widths and observable refusal/precision contracts. Refused factors
 * must preserve both outputs; success must prove the new bounded-count case. */
int main(void)
{
    unsigned long checks = 0;
    uint64_t zero = 17;
    uint64_t one = 29;
    if (!exercise(32, &checks) || !exercise(64, &checks))
    {
        return 1;
    }
    if (fir_sparse_product_bits(0, 0, 0, 32, &zero, &one) || fir_sparse_product_bits(0, 0, 7, 64, &zero, &one) ||
        fir_sparse_product_bits(0, 0, UINT64_MAX, 64, &zero, &one) ||
        fir_sparse_product_bits(0, 0, UINT64_C(1) << 40, 32, &zero, &one) ||
        fir_sparse_product_bits(0, 0, 1, 16, &zero, &one) || zero != 17 || one != 29)
    {
        fputs("FAIL sparse product: refusal modified output\n", stderr);
        return 1;
    }
    if (!fir_sparse_product_bits(UINT32_MAX ^ UINT64_C(7), 0, 3, 32, &zero, &one) ||
        (zero & (UINT32_MAX ^ UINT64_C(31))) != (UINT32_MAX ^ UINT64_C(31)))
    {
        fputs("FAIL sparse product: (x&7)*3 did not prove a checked int32 shift count\n", stderr);
        return 1;
    }
    printf("PASS sparse product bits: %lu concrete modular products, both widths, all factor positions\n", checks);
    return 0;
}
