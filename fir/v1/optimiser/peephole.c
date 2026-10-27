/* Conservative FIR-to-FIR peephole. Never folds dynamic/FP operations, trace,
 * or conversions. Constant branches discard only unexecuted paths. Unsigned arithmetic implements
 * i32 wrap without signed overflow in the compiler itself. */
#include "fir.h"
#include <stdint.h>
#include <string.h>

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


/* Fold loads of private scalar cells with one dominating constant store.
 * module is borrowed and already verified. Return whether copied instructions
 * changed. The unique store must share the cell definition's block; same-block
 * reads before it keep zero-initialization semantics. Other blocks inherit that
 * store by cell dominance. Only immutable literal payloads qualify, so cycles
 * cannot expose a later reuse of a mutable SSA slot. Reference cells stay intact. */
static int fold_constant_cell_loads(FirModule *module)
{
    int changed = 0;
    int index;
    for (index = 0; index < module->count; index++)
    {
        FirInstruction *load = &module->instructions[index];
        FirInstruction *definition;
        FirInstruction *store = NULL;
        int store_index = -1;
        int stores = 0;
        int scan;
        FirOp operation;
        if (load->removed || (load->op != O_BIG_LOAD && load->op != O_BOOL_LOAD))
        {
            continue;
        }
        definition = &module->instructions[module->values[load->a].definition];
        operation = load->op == O_BIG_LOAD ? O_BIG_STORE : O_BOOL_STORE;
        for (scan = 0; scan < module->count; scan++)
        {
            FirInstruction *candidate = &module->instructions[scan];
            if (!candidate->removed && candidate->op == operation && candidate->a == load->a)
            {
                store = candidate;
                store_index = scan;
                stores++;
            }
        }
        if (stores == 1 && store->block == definition->block && (load->block != store->block || index > store_index))
        {
            FirInstruction *value = &module->instructions[module->values[store->b].definition];
            if (value->op == (load->op == O_BIG_LOAD ? O_I64 : O_BOOL))
            {
                load->op = O_COPY;
                load->a = store->b;
                load->b = -1;
                changed = 1;
            }
        }
    }
    return changed;
}


/* Forward private scalar loads from the latest same-block store or load.
 * module is borrowed and verified: scalar cell handles cannot alias or escape.
 * Return whether loads became copies, retaining every constructor/store. Reset
 * facts at block boundaries and unmodelled writes/suspensions; typed scalar
 * writes update only their exact cell. Never forward reference owners, table
 * elements or across backedges. No VM allocation, wait or native pointer occurs. */
static int forward_scalar_loads(FirModule *module)
{
    int known[FIR_LIMIT]; /* Payload SSA IDs, -1 until this block establishes a value. */
    int block = -1;       /* Current linear basic block; facts never cross an edge. */
    int changed = 0;
    int index;
    for (index = 0; index < module->count; index++)
    {
        FirInstruction *in = &module->instructions[index];
        if (in->removed)
        {
            continue;
        }
        if (in->block != block)
        {
            memset(known, -1, sizeof(known));
            block = in->block;
        }
        if (in->op == O_BIG_LOCAL || in->op == O_BOOL_LOCAL)
        {
            known[in->result] = -1;
        }
        else if (in->op == O_BIG_STORE || in->op == O_BOOL_STORE)
        {
            known[in->a] = in->b;
        }
        else if (in->op == O_BIG_LOAD || in->op == O_BOOL_LOAD)
        {
            int cell = in->a;
            if (known[cell] >= 0)
            {
                in->op = O_COPY;
                in->a = known[cell];
                in->b = -1;
                changed = 1;
            }
            else
            {
                known[cell] = in->result;
            }
        }
        else if (fir_effects(in->op) & (FIR_WRITES_MEMORY | FIR_MAY_SUSPEND))
        {
            memset(known, -1, sizeof(known));
        }
    }
    return changed;
}


/* Remove scalar writes overwritten before observation in the same block.
 * module is borrowed and verified; scalar cell handles cannot alias or escape.
 * Return whether stores were retired. Walk backward, tracking the next write
 * to each exact cell. Loads/constructors end that cell's proof, and unrelated
 * writes, suspension or possible exceptions end all proofs. Never remove the
 * final store, reference publication or a payload's own side effects. No runtime
 * memory, ownership, wait or cross-block inference is involved. */
static int remove_overwritten_scalar_stores(FirModule *module)
{
    unsigned char overwritten[FIR_LIMIT]; /* A later unobserved write exists for this cell. */
    int block = -1;                       /* Facts are confined to one basic block. */
    int changed = 0;
    int index;
    for (index = module->count - 1; index >= 0; index--)
    {
        FirInstruction *in = &module->instructions[index];
        if (in->removed)
        {
            continue;
        }
        if (in->block != block)
        {
            memset(overwritten, 0, sizeof(overwritten));
            block = in->block;
        }
        if (in->op == O_BIG_STORE || in->op == O_BOOL_STORE)
        {
            if (overwritten[in->a])
            {
                in->removed = 1;
                changed = 1;
            }
            overwritten[in->a] = 1;
        }
        else if (in->op == O_BIG_LOAD || in->op == O_BOOL_LOAD)
        {
            overwritten[in->a] = 0;
        }
        else if (in->op == O_BIG_LOCAL || in->op == O_BOOL_LOCAL)
        {
            overwritten[in->result] = 0;
        }
        else if (fir_effects(in->op) & (FIR_WRITES_MEMORY | FIR_MAY_SUSPEND | FIR_MAY_THROW))
        {
            memset(overwritten, 0, sizeof(overwritten));
        }
    }
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
        changed = fold_constant_cell_loads(m);
        changed |= forward_scalar_loads(m);
        changed |= remove_overwritten_scalar_stores(m);
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
            if ((in->op == O_I32_SHL || in->op == O_I32_SHR) && a->op == O_I32 && b->op == O_I32 && b->integer >= 0 &&
                b->integer < 32)
            {
                unsigned count = (unsigned)b->integer;
                uint32_t bits = (uint32_t)a->integer;
                if (in->op == O_I32_SHL)
                {
                    bits <<= count;
                }
                else if (count)
                {
                    bits = (bits >> count) | (a->integer < 0 ? UINT32_MAX << (32U - count) : 0);
                }
                in->integer = signed_bits(bits);
                in->op = O_I32;
                in->a = in->b = -1;
                changed = 1;
            }
            else if ((in->op == O_STRING_EQ || in->op == O_STRING_LT) && a->op == O_STRING && b->op == O_STRING)
            {
                /* Valid UTF-8 byte ordering matches Unicode scalar ordering. No locale,
                 * case folding or normalization participates in either comparison. */
                int compared = strcmp(a->text, b->text);
                in->integer = in->op == O_STRING_LT ? compared < 0 : compared == 0;
                in->op = O_BOOL;
                in->a = in->b = -1;
                changed = 1;
            }
            else if (in->op == O_TO_INT && (a->op == O_I64 || a->op == O_I32 || a->op == O_BOOL))
            {
                in->integer = signed_bits((uint32_t)(a->op == O_I64 ? (uint64_t)a->big : (uint32_t)a->integer));
                in->op = O_I32;
                in->a = in->b = -1;
                changed = 1;
            }
            else if (in->op == O_TO_INT && m->values[in->a].type == T_I32)
            {
                in->op = O_COPY;
                changed = 1;
            }
            else if (in->op == O_TO_BIG && (a->op == O_I32 || a->op == O_BOOL))
            {
                in->big = a->integer;
                in->op = O_I64;
                in->a = in->b = -1;
                changed = 1;
            }
            else if ((in->op == O_BIG_SHL || in->op == O_BIG_SHR) && a->op == O_I64 && b->op == O_I32 &&
                     b->integer >= 0 && b->integer < 64)
            {
                unsigned count = (unsigned)b->integer;
                uint64_t bits = (uint64_t)a->big;
                if (in->op == O_BIG_SHL)
                {
                    bits <<= count;
                }
                else if (count)
                {
                    bits = (bits >> count) | (a->big < 0 ? UINT64_MAX << (64U - count) : 0);
                }
                in->big = signed_big_bits(bits);
                in->op = O_I64;
                in->a = in->b = -1;
                changed = 1;
            }
            else if (in->op == O_TO_BIG && m->values[in->a].type == T_I64)
            {
                /* An explicit same-type big cast copies its scalar; text/key conversions
                 * keep their allocation/error effects and are never rewritten here. */
                in->op = O_COPY;
                changed = 1;
            }
            else if ((in->op == O_BIG_LT || in->op == O_BIG_EQ) && a->op == O_I64 && b->op == O_I64)
            {
                in->integer = in->op == O_BIG_LT ? a->big < b->big : a->big == b->big;
                in->op = O_BOOL;
                in->a = in->b = -1;
                changed = 1;
            }
            else if (in->op >= O_BIG_DIV && in->op <= O_BIG_NOT && a->op == O_I64 &&
                     (in->op == O_BIG_NOT || b->op == O_I64) &&
                     (in->op == O_BIG_NOT || (in->op != O_BIG_DIV && in->op != O_BIG_MOD) || b->big != 0))
            {
                uint64_t x = (uint64_t)a->big;
                uint64_t y = in->op == O_BIG_NOT ? 0 : (uint64_t)b->big;
                if (in->op == O_BIG_DIV || in->op == O_BIG_MOD)
                {
                    in->big = a->big == INT64_MIN && b->big == -1
                                  ? (in->op == O_BIG_DIV ? INT64_MIN : 0)
                                  : (in->op == O_BIG_DIV ? a->big / b->big : a->big % b->big);
                }
                else
                {
                    uint64_t bits = in->op == O_BIG_AND   ? x & y
                                    : in->op == O_BIG_OR  ? x | y
                                    : in->op == O_BIG_XOR ? x ^ y
                                                          : ~x;
                    in->big = signed_big_bits(bits);
                }
                in->op = O_I64;
                in->a = in->b = -1;
                changed = 1;
            }
            else if (in->op >= O_BIG_ADD && in->op <= O_BIG_NEG && a->op == O_I64 &&
                     (in->op == O_BIG_NEG || b->op == O_I64))
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
            else if ((in->op == O_I32_DIV || in->op == O_I32_MOD) && a->op == O_I32 && b->op == O_I32 &&
                     b->integer != 0)
            {
                int64_t left = a->integer;
                int64_t right = b->integer;
                int64_t value = in->op == O_I32_DIV ? left / right : left % right;
                in->integer = signed_bits((uint32_t)(uint64_t)value);
                in->op = O_I32;
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
                 (in->op >= O_BIG_ADD && in->op <= O_TO_INT)) &&
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
