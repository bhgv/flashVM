/* Conservative bit facts for verified immutable int32 SSA values. */
#include "fir.h"
#include <string.h>


/* Derive known zero/one bits without reading cells or evaluating producers.
 * module is borrowed verified IR; facts is disjoint caller-owned scratch.
 * Clear stale facts on every call. Only constants/copy/AND/OR/XOR propagate;
 * everything else is unknown. Monotone scans handle non-topological blocks.
 * At most value_count scans suffice for an acyclic SSA definition graph; an
 * incomplete proof remains conservative. No allocation, callback or wait occurs.
 * Facts describe successful evaluation and never certify producer effects. */
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
            uint32_t zero = 0;
            uint32_t one = 0;
            uint32_t az, ao, bz, bo;
            if (in->removed || in->result < 0 || in->type != T_I32)
            {
                continue;
            }
            if (in->op == O_I32)
            {
                one = (uint32_t)in->integer;
                zero = ~one;
            }
            else if (in->op == O_COPY)
            {
                zero = facts->zero[in->a];
                one = facts->one[in->a];
            }
            else if (in->op == O_I32_AND || in->op == O_I32_OR || in->op == O_I32_XOR)
            {
                az = facts->zero[in->a];
                ao = facts->one[in->a];
                bz = facts->zero[in->b];
                bo = facts->one[in->b];
                if (in->op == O_I32_AND)
                {
                    zero = az | bz;
                    one = ao & bo;
                }
                else if (in->op == O_I32_OR)
                {
                    zero = az & bz;
                    one = ao | bo;
                }
                else
                {
                    zero = (az & bz) | (ao & bo);
                    one = (az & bo) | (ao & bz);
                }
            }
            if (zero != facts->zero[in->result] || one != facts->one[in->result])
            {
                facts->zero[in->result] = zero;
                facts->one[in->result] = one;
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
