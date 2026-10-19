/* Conservative FIR-to-FIR peephole. Never folds dynamic/FP operations, trace,
 * branches or conversions. Unsigned arithmetic implements i32 wrap without
 * signed overflow in the compiler itself. */
#include "fir.h"
#include <stdint.h>

/* Interpret a modulo-2^32 result without implementation-defined unsigned casts. */
static int32_t signed_bits(uint32_t bits)
{
    return bits <= INT32_MAX ? (int32_t)bits :
        (int32_t)(-1 - (int32_t)(UINT32_MAX - bits));
}
/* Run bounded local rewrites until stable, checking the IR at both boundaries. */
int fir_optimise(FirModule *m, char *error, size_t cap)
{
    int changed, i, j;
    if (!fir_verify(m, error, cap)) return 0;
    do {
        changed = 0;
        for (i = 0; i < m->count; ++i) {
            FirInstruction *in = &m->instructions[i];
            FirInstruction *a, *b;
            if (in->removed) continue;
            a = in->a < 0 ? 0 : &m->instructions[m->values[in->a].definition];
            b = in->b < 0 ? 0 : &m->instructions[m->values[in->b].definition];
            if (in->op >= O_ADD && in->op <= O_EQ && a->op == O_I32 && b->op == O_I32) {
                uint32_t x = (uint32_t)a->integer, y = (uint32_t)b->integer;
                if (in->op == O_LT || in->op == O_EQ) {
                    in->integer = in->op == O_LT ? a->integer < b->integer : x == y;
                    in->op = O_BOOL;
                } else {
                    uint32_t value = in->op == O_ADD ? x + y : in->op == O_SUB ? x - y : x * y;
                    in->integer = signed_bits(value);
                    in->op = O_I32;
                }
                in->a = in->b = -1;
                changed = 1;
            } else if ((in->op == O_ADD || in->op == O_SUB) && b->op == O_I32 && !b->integer) {
                in->op = O_COPY; in->b = -1; changed = 1;
            }
            if (in->op == O_COPY) {
                for (j = 0; j < m->count; ++j) {
                    if (m->instructions[j].a == in->result) m->instructions[j].a = in->a;
                    if (m->instructions[j].b == in->result) m->instructions[j].b = in->a;
                }
                in->removed = 1; changed = 1;
            }
        }
        /* DCE only for operations whose total, effect-free behaviour is part
         * of this version's contract. Dynamic addition may allocate/throw. */
        for (i = m->count - 1; i >= 0; --i) {
            FirInstruction *in = &m->instructions[i];
            int used = 0;
            if (in->removed || in->result < 0 || in->op > O_FMUL) continue;
            for (j = 0; j < m->count; ++j) if (!m->instructions[j].removed &&
                (m->instructions[j].a == in->result || m->instructions[j].b == in->result)) used = 1;
            if (!used) { in->removed = 1; changed = 1; }
        }
    } while (changed);
    return fir_verify(m, error, cap);
}
