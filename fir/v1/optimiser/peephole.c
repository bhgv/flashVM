/* Conservative FIR-to-FIR peephole. Never folds dynamic/FP operations, trace,
 * or conversions. Constant branches discard only unexecuted paths. Unsigned arithmetic implements
 * i32 wrap without signed overflow in the compiler itself. */
#include "fir.h"
#include <stdint.h>

/* Interpret a modulo-2^32 result without implementation-defined unsigned casts. */
static int32_t signed_bits(uint32_t bits)
{
    return bits <= INT32_MAX ? (int32_t)bits : (int32_t)(-1 - (int32_t)(UINT32_MAX - bits));
}

/* Interpret modulo-2^64 arithmetic without implementation-defined unsigned casts.
 * bits is a scalar payload; conversion is total and has no allocation or effects. */
static int64_t signed_big_bits(uint64_t bits)
{
    return bits <= INT64_MAX ? (int64_t)bits : -1 - (int64_t)(UINT64_MAX - bits);
}

/* Fold known branches, find live blocks (including cycles), and compact label
 * indices. Instruction/value indices stay stable for definition references. */
static int simplify_cfg(FirModule *m)
{
    unsigned char live[FIR_BLOCKS] = {0};
    int mapping[FIR_BLOCKS], i, b, again, changed = 0, count = 0;
    for (i = 0; i < m->count; ++i)
    {
        FirInstruction *in = &m->instructions[i];
        if (!in->removed && in->op == O_IF)
        {
            FirInstruction *value = &m->instructions[m->values[in->a].definition];
            if (value->op == O_BOOL)
            {
                in->yes = value->integer ? in->yes : in->no;
                in->no = in->a = -1;
                in->op = O_JUMP;
                changed = 1;
            }
        }
    }
    live[m->instructions[0].block] = 1;
    do
    {
        again = 0;
        for (i = 0; i < m->count; ++i)
        {
            FirInstruction *in = &m->instructions[i];
            if (in->removed || !live[in->block])
            {
                continue;
            }
            if (in->yes >= 0 && !live[in->yes])
            {
                live[in->yes] = 1;
                again = 1;
            }
            if (in->no >= 0 && !live[in->no])
            {
                live[in->no] = 1;
                again = 1;
            }
        }
    } while (again);
    for (b = 0; b < m->block_count; ++b)
    {
        mapping[b] = live[b] ? count++ : -1;
        if (live[b])
        {
            m->blocks[mapping[b]] = m->blocks[b];
        }
        else
        {
            changed = 1;
        }
    }
    for (i = 0; i < m->count; ++i)
    {
        FirInstruction *in = &m->instructions[i];
        if (!live[in->block])
        {
            in->removed = 1;
        }
        /* Removed instructions may still be visited in later iterations. Keep
         * their block field valid without reviving them or their operands. */
        in->block = mapping[in->block] < 0 ? 0 : mapping[in->block];
        if (in->yes >= 0)
        {
            in->yes = mapping[in->yes];
        }
        if (in->no >= 0)
        {
            in->no = mapping[in->no];
        }
    }
    m->block_count = count;
    return changed;
}

/* Run bounded local rewrites until stable, checking the IR at both boundaries. */
int fir_optimise(FirModule *m, char *error, size_t cap)
{
    int changed, i, j;
    if (!fir_verify(m, error, cap))
    {
        return 0;
    }
    do
    {
        changed = 0;
        for (i = 0; i < m->count; ++i)
        {
            FirInstruction *in = &m->instructions[i];
            FirInstruction *a, *b;
            if (in->removed)
            {
                continue;
            }
            a = in->a < 0 ? 0 : &m->instructions[m->values[in->a].definition];
            b = in->b < 0 ? 0 : &m->instructions[m->values[in->b].definition];
            if (in->op >= O_BIG_ADD && in->op <= O_BIG_NEG && a->op == O_I64 && (in->op == O_BIG_NEG || b->op == O_I64))
            {
                uint64_t x = (uint64_t)a->big;
                uint64_t y = in->op == O_BIG_NEG ? 0 : (uint64_t)b->big;
                uint64_t bits = in->op == O_BIG_ADD   ? x + y
                                : in->op == O_BIG_SUB ? x - y
                                : in->op == O_BIG_MUL ? x * y
                                                      : 0U - x;
                in->big = signed_big_bits(bits);
                in->op = O_I64;
                in->a = in->b = -1;
                changed = 1;
            }
            else if (in->op >= O_ADD && in->op <= O_EQ && a->op == O_I32 && b->op == O_I32)
            {
                uint32_t x = (uint32_t)a->integer, y = (uint32_t)b->integer;
                if (in->op == O_LT || in->op == O_EQ)
                {
                    in->integer = in->op == O_LT ? a->integer < b->integer : x == y;
                    in->op = O_BOOL;
                }
                else
                {
                    uint32_t value = in->op == O_ADD ? x + y : in->op == O_SUB ? x - y : x * y;
                    in->integer = signed_bits(value);
                    in->op = O_I32;
                }
                in->a = in->b = -1;
                changed = 1;
            }
            else if ((in->op == O_ADD || in->op == O_SUB) && b->op == O_I32 && !b->integer)
            {
                in->op = O_COPY;
                in->b = -1;
                changed = 1;
            }
            /* Pool exact big constants and reuse identical typed calculations within one block.
             * Operand order is preserved, including strict floating arithmetic. */
            if ((in->op == O_I64 || (in->op >= O_ADD && in->op <= O_FDIV) ||
                 (in->op >= O_BIG_ADD && in->op <= O_BIG_NEG)) &&
                !fir_effects(in->op))
            {
                for (j = 0; j < i; ++j)
                {
                    FirInstruction *old = &m->instructions[j];
                    if (!old->removed && old->block == in->block && old->op == in->op && old->a == in->a &&
                        old->b == in->b && old->type == in->type && (in->op != O_I64 || old->big == in->big))
                    {
                        in->op = O_COPY;
                        in->a = old->result;
                        in->b = -1;
                        changed = 1;
                        break;
                    }
                }
            }
            if (in->op == O_COPY)
            {
                for (j = 0; j < m->count; ++j)
                {
                    if (m->instructions[j].a == in->result)
                    {
                        m->instructions[j].a = in->a;
                    }
                    if (m->instructions[j].b == in->result)
                    {
                        m->instructions[j].b = in->a;
                    }
                }
                in->removed = 1;
                changed = 1;
            }
        }
        changed |= simplify_cfg(m);
        /* DCE only for operations whose total, effect-free behaviour is part
         * of this version's contract. Dynamic addition may allocate/throw. */
        for (i = m->count - 1; i >= 0; --i)
        {
            FirInstruction *in = &m->instructions[i];
            int used = 0;
            if (in->removed || in->result < 0 || fir_effects(in->op) != 0)
            {
                continue;
            }
            for (j = 0; j < m->count; ++j)
            {
                if (!m->instructions[j].removed &&
                    (m->instructions[j].a == in->result || m->instructions[j].b == in->result))
                {
                    used = 1;
                }
            }
            if (!used)
            {
                in->removed = 1;
                changed = 1;
            }
        }
    } while (changed);
    return fir_verify(m, error, cap);
}
