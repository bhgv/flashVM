/* Conservative bit facts for verified immutable int32 and big SSA values. */
#include "fir.h"
#include "integer_product_bits.h"
#include "integer_division_bits.h"
#include <string.h>


/* Count the contiguous known-zero low bits of a borrowed mask, up to width.
 * width is32 or64. Return width for an entirely zero value. No signed shifts,
 * allocation, IR mutation or callbacks occur; unknown bits stop the proof. */
static unsigned trailing_zero_facts(uint64_t zero, unsigned width)
{
    unsigned count = 0;
    while (count < width && (zero & (UINT64_C(1) << count)))
    {
        count++;
    }
    return count;
}


/* Derive known bits without reading cells or evaluating operand producers.
 * module is borrowed verified IR; facts is disjoint caller-owned scratch.
 * Clear stale facts, then follow int32/big literals, copies, bit operations and
 * shifts with proven legal int32 counts, modular add/sub/neg carry facts,
 * scalar casts, sparse/low-bit modular products and signed power-of-two division.
 * Everything else is unknown.
 * Monotone bounded scans handle non-topological definitions; an incomplete
 * proof stays conservative. No allocation, callbacks or waiting occurs. These
 * successful-result facts never certify that operand producers cannot throw. */
void fir_integer_facts(const FirModule *module, FirIntegerFacts *facts)
{
    int pass;
    memset(facts, 0, sizeof(*facts));
    for (pass = 0; pass < module->value_count; pass++)
    {
        int index;
        int changed = 0;
        for (index = 0; index < module->count; index++)
        {
            const FirInstruction *in = &module->instructions[index];
            uint64_t zero = 0;
            uint64_t one = 0;
            uint64_t az = 0;
            uint64_t ao = 0;
            uint64_t bz = 0;
            uint64_t bo = 0;
            uint64_t mask;
            unsigned width;
            int wide;
            if (in->removed || in->result < 0 || (in->type != T_I32 && in->type != T_I64))
            {
                continue;
            }
            wide = in->type == T_I64;
            width = wide ? 64 : 32;
            mask = wide ? UINT64_MAX : UINT32_MAX;
            if (in->a >= 0)
            {
                az = wide ? facts->big_zero[in->a] : facts->zero[in->a];
                ao = wide ? facts->big_one[in->a] : facts->one[in->a];
            }
            if (in->b >= 0)
            {
                bz = wide ? facts->big_zero[in->b] : facts->zero[in->b];
                bo = wide ? facts->big_one[in->b] : facts->one[in->b];
            }
            if (in->op == O_I32 || in->op == O_I64)
            {
                one = wide ? (uint64_t)in->big : (uint32_t)in->integer;
                zero = ~one & mask;
            }
            else if (in->op == O_COPY)
            {
                zero = az;
                one = ao;
            }
            else if (in->op == O_TO_INT || in->op == O_TO_BIG)
            {
                FirType source = module->values[in->a].type;
                if (source == T_BOOL)
                {
                    zero = mask & ~UINT64_C(1);
                }
                else if (source == T_I32)
                {
                    zero = facts->zero[in->a];
                    one = facts->one[in->a];
                    if (wide)
                    {
                        uint64_t high = UINT64_MAX ^ (uint64_t)UINT32_MAX;
                        if (zero & UINT32_C(0x80000000))
                        {
                            zero |= high;
                        }
                        else if (one & UINT32_C(0x80000000))
                        {
                            one |= high;
                        }
                    }
                }
                else if (source == T_I64)
                {
                    zero = facts->big_zero[in->a] & mask;
                    one = facts->big_one[in->a] & mask;
                }
                /* String/key conversion producers remain outside this proof. */
            }
            else if (in->op == O_ADD || in->op == O_SUB || in->op == O_BIG_ADD || in->op == O_BIG_SUB)
            {
                fir_bit_arithmetic(az, ao, bz, bo, width, in->op == O_SUB || in->op == O_BIG_SUB, &zero, &one);
            }
            else if (in->op == O_MUL || in->op == O_BIG_MUL)
            {
                unsigned left = trailing_zero_facts(az, width);
                unsigned right = trailing_zero_facts(bz, width);
                unsigned count = left + right;
                /* One/two set bits are shifts followed by a modular addition.
                 * Require complete factor knowledge; retain the conservative
                 * divisibility fallback for all other constants/dynamic factors. */
                if (((bz | bo) == mask && fir_sparse_product_bits(az, ao, bo, width, &zero, &one)) ||
                    ((az | ao) == mask && fir_sparse_product_bits(bz, bo, ao, width, &zero, &one)))
                {
                    /* The helper writes successful-result facts, never an IR rewrite. */
                }
                else
                {
                    /* Divisibility adds under multiplication, including wraparound.
                     * Saturate before shifting so a full-width proof never shifts64. */
                    zero = count >= width ? mask : (UINT64_C(1) << count) - 1;
                    if (count < width && (ao & (UINT64_C(1) << left)) && (bo & (UINT64_C(1) << right)))
                    {
                        one = UINT64_C(1) << count;
                    }
                }
            }
            else if (in->op == O_I32_DIV || in->op == O_I32_MOD ||
                     in->op == O_BIG_DIV || in->op == O_BIG_MOD)
            {
                if ((bz | bo) == mask)
                {
                    fir_power_division_bits(az, ao, bo, width,
                                            in->op == O_I32_MOD || in->op == O_BIG_MOD,
                                            &zero, &one);
                }
            }
            else if (in->op == O_BIG_NEG)
            {
                fir_bit_arithmetic(mask, 0, az, ao, width, 1, &zero, &one);
            }
            else if (in->op == O_I32_AND || in->op == O_BIG_AND)
            {
                zero = az | bz;
                one = ao & bo;
            }
            else if (in->op == O_I32_OR || in->op == O_BIG_OR)
            {
                zero = az & bz;
                one = ao | bo;
            }
            else if (in->op == O_I32_XOR || in->op == O_BIG_XOR)
            {
                zero = (az & bz) | (ao & bo);
                one = (az & bo) | (ao & bz);
            }
            else if (in->op == O_BIG_NOT)
            {
                zero = ao;
                one = az;
            }
            else if (in->op == O_I32_SHL || in->op == O_I32_SHR || in->op == O_BIG_SHL || in->op == O_BIG_SHR)
            {
                uint32_t count = facts->one[in->b];
                if ((facts->zero[in->b] | count) == UINT32_MAX && count < width)
                {
                    if (count == 0)
                    {
                        zero = az;
                        one = ao;
                    }
                    else if (in->op == O_I32_SHL || in->op == O_BIG_SHL)
                    {
                        zero = ((az << count) | ((UINT64_C(1) << count) - 1)) & mask;
                        one = (ao << count) & mask;
                    }
                    else
                    {
                        uint64_t sign = UINT64_C(1) << (width - 1);
                        uint64_t fill = mask ^ (mask >> count);
                        zero = az >> count;
                        one = ao >> count;
                        if (az & sign)
                        {
                            zero |= fill;
                        }
                        else if (ao & sign)
                        {
                            one |= fill;
                        }
                    }
                }
            }
            if (wide)
            {
                if (zero != facts->big_zero[in->result] || one != facts->big_one[in->result])
                {
                    facts->big_zero[in->result] = zero;
                    facts->big_one[in->result] = one;
                    changed = 1;
                }
            }
            else if (zero != facts->zero[in->result] || one != facts->one[in->result])
            {
                facts->zero[in->result] = (uint32_t)zero;
                facts->one[in->result] = (uint32_t)one;
                changed = 1;
            }
        }
        if (!changed)
        {
            break;
        }
    }
}


/* Return whether successful int32 SSA evaluation has a provably set bit.
 * facts is a fresh borrowed analysis; value is a verified int32 SSA index.
 * Unknown and exact zero return zero. No ownership or producer effects change. */
int fir_integer_nonzero(const FirIntegerFacts *facts, int value)
{
    return facts->one[value] != 0;
}


/* Return whether fresh borrowed facts prove a successful big SSA value nonzero.
 * value is a verified big index. Unknown/zero returns zero; no producer effect,
 * ownership, memory, scheduling or evaluation is changed by this query. */
int fir_big_nonzero(const FirIntegerFacts *facts, int value)
{
    return facts->big_one[value] != 0;
}


/* Prove an int32 SSA count lies in [0,width-1] for width32 or width64.
 * facts/value follow the analysis contract. Return zero for unsupported width
 * or incomplete high-bit proof. A successful proof does not evaluate a count
 * or authorize removing errors in its producer. No VM state is accessed. */
int fir_integer_shift_count(const FirIntegerFacts *facts, int value, unsigned width)
{
    uint32_t outside;
    if (width != 32 && width != 64)
    {
        return 0;
    }
    outside = ~(uint32_t)(width - 1);
    return (facts->zero[value] & outside) == outside;
}
