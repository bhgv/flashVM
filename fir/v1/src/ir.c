/* FIR 1.0 SSA verifier and canonical printer. Parsing lives in the
 * Coco/R grammar compiler/fir1.atg and its semantic builder. */
#include "fir.h"
#include <string.h>
#include <stdarg.h>
#include <math.h>
/* Format a bounded verifier diagnostic and return failure. */
static int fail(char *error, size_t cap, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(error, cap, format, args);
    va_end(args);
    return 0;
}
/* Public IR spelling has the same bounded ASCII grammar as .fir input. */
static int symbol_name(const char *text, char prefix)
{
    size_t i;
    if (!memchr(text, 0, FIR_TEXT) || text[0] != prefix || !text[1])
        return 0;
    for (i = 1; text[i]; ++i)
    {
        unsigned ch = (unsigned char)text[i];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_' ||
              (i > 1 && ((ch >= '0' && ch <= '9') || ch == '.'))))
            return 0;
    }
    return 1;
}
static const char *const types[] = {"void", "i32", "f64", "bool", "string", "any", "channel"};
static const char *const ops[] = {"const.i32",
                                  "const.f64",
                                  "const.bool",
                                  "const.string",
                                  "copy",
                                  "i32.add.wrap",
                                  "i32.sub.wrap",
                                  "i32.mul.wrap",
                                  "i32.lt.signed",
                                  "i32.eq",
                                  "f64.add",
                                  "f64.sub",
                                  "f64.mul",
                                  "f64.div",
                                  "as.add",
                                  "as.to_number",
                                  "limbo.chan_create",
                                  "limbo.chan_receive",
                                  "trace",
                                  "limbo.chan_send",
                                  "limbo.chan_close",
                                  "if",
                                  "jump",
                                  "return"};
/* All blocks must be reachable and terminated. Requiring reachability keeps
 * the initial verifier small and makes dominance precise even for cycles. */
/* Check CFG structure, compute dominators, then validate each SSA operand/type. */
int fir_verify(FirModule *m, char *error, size_t cap)
{
    unsigned char edge[FIR_BLOCKS][FIR_BLOCKS] = {{0}};
    unsigned char dom[FIR_BLOCKS][FIR_BLOCKS], reachable[FIR_BLOCKS] = {0};
    int first[FIR_BLOCKS], last[FIR_BLOCKS], terminated[FIR_BLOCKS] = {0};
    int i, b, j, k, changed, entry;
    /* Public C callers can build IR themselves; reject indices before any
     * table lookup. Removed instructions retain definitions but no live CFG. */
    if (!m || m->count < 1 || m->count > FIR_LIMIT || m->value_count < 0 ||
        m->value_count > FIR_LIMIT || m->block_count < 1 || m->block_count > FIR_BLOCKS)
        return fail(error, cap, "invalid IR table counts");
    if ((unsigned)m->source_language > FIR_LIMBO || (unsigned)m->source_semantics > FIR_LIMBO_I32 ||
        (!m->source_language != !m->source_semantics) ||
        (m->source_language &&
         m->source_semantics !=
             (m->source_language == FIR_LIMBO ? FIR_LIMBO_I32 : FIR_AVM1_NUMERIC)))
        return fail(error, cap, "incompatible source language/semantics profile");
    if (!symbol_name(m->module, '@'))
        return fail(error, cap, "invalid module spelling");
    for (b = 0; b < m->block_count; ++b)
        if (!symbol_name(m->blocks[b].name, '^'))
            return fail(error, cap, "unterminated block name");
    for (i = 0; i < m->value_count; ++i)
    {
        int d = m->values[i].definition;
        if (!symbol_name(m->values[i].name, '%') || d < -1 || d >= m->count ||
            (unsigned)m->values[i].type > T_CHANNEL ||
            (d >= 0 &&
             (m->instructions[d].result != i || m->instructions[d].type != m->values[i].type)))
            return fail(error, cap, "invalid value definition table");
    }
    for (i = 0; i < m->count; ++i)
    {
        FirInstruction *in = &m->instructions[i];
        int unary, binary;
        if ((unsigned)in->op > O_RETURN || (unsigned)in->type > T_CHANNEL || in->block < 0 ||
            in->block >= m->block_count || in->result < -1 || in->result >= m->value_count ||
            in->a < -1 || in->a >= m->value_count || in->b < -1 || in->b >= m->value_count ||
            in->yes < -1 || in->yes >= m->block_count || in->no < -1 || in->no >= m->block_count ||
            !memchr(in->text, 0, FIR_TEXT))
            return fail(error, cap, "invalid instruction fields");
        if (in->removed)
            continue;
        if ((in->op == O_BOOL && in->integer != 0 && in->integer != 1) ||
            (in->op == O_F64 && !isfinite(in->real)))
            return fail(error, cap, "invalid constant payload");
        unary = in->op == O_COPY || in->op == O_NUMBER || in->op == O_RECEIVE ||
                in->op == O_TRACE || in->op == O_CLOSE || in->op == O_IF;
        binary = (in->op >= O_ADD && in->op <= O_ASADD) || in->op == O_SEND;
        if ((in->a >= 0) != (unary || binary) || (in->b >= 0) != binary ||
            (in->yes >= 0) != (in->op == O_IF || in->op == O_JUMP) ||
            (in->no >= 0) != (in->op == O_IF) ||
            (in->result >= 0 && m->values[in->result].definition != i))
            return fail(error, cap, "invalid operand/target arity");
    }
    for (b = 0; b < m->block_count; ++b)
    {
        first[b] = last[b] = -1;
        if (!m->blocks[b].defined)
            return fail(error, cap, "undefined block %s", m->blocks[b].name);
    }
    for (i = 0; i < m->count; ++i)
    {
        FirInstruction *in = &m->instructions[i];
        if (in->removed)
            continue;
        b = in->block;
        if (terminated[b])
            return fail(error, cap, "line %d: instruction after terminator", in->line);
        if (first[b] < 0)
            first[b] = i;
        last[b] = i;
        if ((in->op < O_TRACE) != (in->result >= 0))
            return fail(error, cap, "line %d: wrong result arity", in->line);
        if (in->op >= O_IF)
        {
            terminated[b] = 1;
            if (in->yes >= 0)
                edge[b][in->yes] = 1;
            if (in->no >= 0)
                edge[b][in->no] = 1;
        }
    }
    entry = m->instructions[0].block;
    reachable[entry] = 1;
    do
    {
        changed = 0;
        for (b = 0; b < m->block_count; ++b)
            if (reachable[b])
                for (j = 0; j < m->block_count; ++j)
                    if (edge[b][j] && !reachable[j])
                    {
                        reachable[j] = 1;
                        changed = 1;
                    }
    } while (changed);
    for (b = 0; b < m->block_count; ++b)
    {
        if (first[b] < 0 || !terminated[b])
            return fail(error, cap, "empty/unterminated block %s", m->blocks[b].name);
        if (!reachable[b])
            return fail(error, cap, "unreachable block %s", m->blocks[b].name);
        for (j = 0; j < m->block_count; ++j)
            dom[b][j] = (unsigned char)(b == entry ? j == entry : 1);
    }
    do
    {
        changed = 0;
        for (b = 0; b < m->block_count; ++b)
            if (b != entry)
                for (j = 0; j < m->block_count; ++j)
                {
                    int value = 1;
                    for (k = 0; k < m->block_count; ++k)
                        if (edge[k][b])
                            value &= dom[k][j];
                    if (j == b)
                        value = 1;
                    if (dom[b][j] != value)
                    {
                        dom[b][j] = (unsigned char)value;
                        changed = 1;
                    }
                }
    } while (changed);
    for (i = 0; i < m->count; ++i)
    {
        FirInstruction *in = &m->instructions[i];
        FirType a = T_VOID, c = T_VOID, expected = T_VOID;
        int operands[2] = {in->a, in->b};
        if (in->removed)
            continue;
        for (j = 0; j < 2; ++j)
            if (operands[j] >= 0)
            {
                int d = m->values[operands[j]].definition;
                if (d < 0 || m->instructions[d].removed ||
                    !dom[in->block][m->instructions[d].block] ||
                    (in->block == m->instructions[d].block && d >= i))
                    return fail(error, cap, "line %d: undefined/non-dominating operand", in->line);
            }
        if (in->a >= 0)
            a = m->values[in->a].type;
        if (in->b >= 0)
            c = m->values[in->b].type;
        switch (in->op)
        {
        case O_I32:
            expected = T_I32;
            break;
        case O_F64:
            expected = T_F64;
            break;
        case O_BOOL:
            expected = T_BOOL;
            break;
        case O_STRING:
            expected = T_STRING;
            break;
        case O_COPY:
            expected = a;
            break;
        case O_ADD:
        case O_SUB:
        case O_MUL:
        case O_LT:
        case O_EQ:
            if (a != T_I32 || c != T_I32)
                goto bad_type;
            expected = in->op >= O_LT ? T_BOOL : T_I32;
            break;
        case O_FADD:
        case O_FSUB:
        case O_FMUL:
        case O_FDIV:
            if (a != T_F64 || c != T_F64)
                goto bad_type;
            expected = T_F64;
            break;
        case O_ASADD:
            if (!a || !c || a == T_CHANNEL || c == T_CHANNEL)
                goto bad_type;
            expected = T_ANY;
            break;
        case O_NUMBER:
            if (!a || a == T_CHANNEL)
                goto bad_type;
            expected = T_F64;
            break;
        case O_CHANNEL:
            if (!m->channels)
                return fail(error, cap, "channels require requires flashvm.channels");
            expected = T_CHANNEL;
            break;
        case O_RECEIVE:
            if (a != T_CHANNEL || !m->channels)
                goto bad_type;
            expected = T_ANY;
            break;
        case O_SEND:
            if (a != T_CHANNEL || c < T_I32 || c > T_ANY || !m->channels)
                goto bad_type;
            break;
        case O_CLOSE:
            if (a != T_CHANNEL || !m->channels)
                goto bad_type;
            break;
        case O_TRACE:
            if (!a || a == T_CHANNEL)
                goto bad_type;
            break;
        case O_IF:
            if (a != T_BOOL)
                goto bad_type;
            break;
        default:
            break;
        }
        if (expected != in->type)
            goto bad_type;
        continue;
    bad_type:
        return fail(error, cap, "line %d: operand/result type mismatch", in->line);
    }
    return 1;
}
/* Escape a decoded string into canonical, reparsable FIR spelling. */
static void quoted(FILE *out, const char *text)
{
    const unsigned char *s = (const unsigned char *)text;
    fputc('"', out);
    for (; *s; ++s)
    {
        if (*s == '\n')
            fputs("\\n", out);
        else if (*s == '\r')
            fputs("\\r", out);
        else if (*s == '\t')
            fputs("\\t", out);
        else
        {
            if (*s == '\\' || *s == '"')
                fputc('\\', out);
            fputc(*s, out);
        }
    }
    fputc('"', out);
}
/* Emit live instructions in stable input order, retaining original line notes. */
int fir_print(FILE *out, const FirModule *m)
{
    int i, previous = -1;
    fprintf(out, "FIR 1.0\nmodule %s\n", m->module);
    if (m->source_language)
    {
        static const char *const languages[] = {"", "as1", "as2", "as3", "limbo"};
        fprintf(out, "source_language %s\nsource_semantics %s\n", languages[m->source_language],
                m->source_semantics == FIR_LIMBO_I32 ? "limbo_i32" : "avm1_numeric");
    }
    if (m->channels)
        fputs("requires flashvm.channels\n", out);
    fputs("func @main() -> void {\n", out);
    for (i = 0; i < m->count; ++i)
    {
        const FirInstruction *in = &m->instructions[i];
        if (in->removed)
            continue;
        if (in->block != previous)
        {
            fprintf(out, "%s:\n", m->blocks[in->block].name);
            previous = in->block;
        }
        fputs("    ", out);
        if (in->result >= 0)
            fprintf(out, "%s: %s = ", m->values[in->result].name, types[in->type]);
        fputs(ops[in->op], out);
        if (in->op == O_I32)
            fprintf(out, " %ld", (long)in->integer);
        else if (in->op == O_F64)
            fprintf(out, " %.17g", in->real);
        else if (in->op == O_BOOL)
            fprintf(out, " %s", in->integer ? "true" : "false");
        else if (in->op == O_STRING)
        {
            fputc(' ', out);
            quoted(out, in->text);
        }
        else
        {
            if (in->a >= 0)
                fprintf(out, " %s", m->values[in->a].name);
            if (in->b >= 0)
                fprintf(out, ", %s", m->values[in->b].name);
            if (in->yes >= 0)
                fprintf(out, in->a >= 0 ? ", %s" : " %s", m->blocks[in->yes].name);
            if (in->no >= 0)
                fprintf(out, ", %s", m->blocks[in->no].name);
        }
        fprintf(out, " // input line %d\n", in->line);
    }
    fputs("}\n", out);
    return !ferror(out);
}
