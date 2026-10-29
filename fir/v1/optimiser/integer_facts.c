/* Conservative bit facts for verified immutable int32 SSA values. */
#include "fir.h"
#include <string.h>


/* Derive known bits without reading cells or evaluating operand producers.
 * module is borrowed verified IR; facts is disjoint caller-owned scratch.
 * Clear stale facts, then follow int32/big literals, copies, bit operations and
 * shifts with an exact proven legal int32 count. Everything else is unknown.
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
