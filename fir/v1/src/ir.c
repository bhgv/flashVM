/* FIR 1.0 SSA verifier and canonical printer. Parsing lives in the
 * Coco/R grammar compiler/fir1.atg and its semantic builder. */
#include "fir.h"
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <inttypes.h>

/* Shared bounded UTF-8 validator from fir1_io; no parser or VM state is acquired. */
extern int fir_utf8(const unsigned char *, size_t);

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
    {
        return 0;
    }
    for (i = 1; text[i]; ++i)
    {
        unsigned ch = (unsigned char)text[i];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_' ||
              (i > 1 && ((ch >= '0' && ch <= '9') || ch == '.'))))
        {
            return 0;
        }
    }
    return 1;
}

static const char *const types[] = {"void",
                                    "i32",
                                    "f64",
                                    "bool",
                                    "string",
                                    "any",
                                    "channel",
                                    "i64",
                                    "table.big",
                                    "table.slot",
                                    "big.range",
                                    "keys",
                                    "table.key",
                                    "bool.slot"};
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
                                  "return",
                                  "const.i64",
                                  "table.new",
                                  "table.index",
                                  "table.load",
                                  "table.store",
                                  "table.len",
                                  "range.big",
                                  "table.slice",
                                  "table.keys",
                                  "keys.len",
                                  "keys.at",
                                  "value.type",
                                  "value.big",
                                  "value.text",
                                  "big.add",
                                  "big.sub",
                                  "big.mul",
                                  "big.neg",
                                  "big.div",
                                  "big.mod",
                                  "big.and",
                                  "big.or",
                                  "big.xor",
                                  "big.not",
                                  "big.lt.signed",
                                  "big.eq",
                                  "value.int",
                                  "big.shl",
                                  "big.shr",
                                  "table.count",
                                  "table.has",
                                  "table.delete",
                                  "bool.local",
                                  "bool.store",
                                  "bool.load",
                                  "string.len",
                                  "string.at",
                                  "string.slice",
                                  "i32.div.signed",
                                  "i32.mod.signed",
                                  "string.concat",
                                  "string.eq",
                                  "i32.shl.checked",
                                  "i32.shr.checked",
                                  "string.lt"};

/* All blocks must be reachable and terminated. Requiring reachability keeps
 * the initial verifier small and makes dominance precise even for cycles. */


/* Check CFG structure, compute dominators, then validate each SSA operand/type. */
int fir_verify(FirModule *m, char *error, size_t cap)
{
    unsigned char edge[FIR_BLOCKS][FIR_BLOCKS] = {{0}};
    unsigned char dom[FIR_BLOCKS][FIR_BLOCKS], reachable[FIR_BLOCKS] = {0};
    int first[FIR_BLOCKS], last[FIR_BLOCKS], terminated[FIR_BLOCKS] = {0};
    int i, b, j, k, changed, entry;
    int previous = -1; /* Live instructions must form contiguous blocks beginning with the entry. */
    /* Public C callers can build IR themselves; reject indices before any
     * table lookup. Removed instructions retain definitions but no live CFG. */
    if (!m || m->count < 1 || m->count > FIR_LIMIT || m->value_count < 0 || m->value_count > FIR_LIMIT ||
        m->block_count < 1 || m->block_count > FIR_BLOCKS)
    {
        return fail(error, cap, "invalid IR table counts");
    }
    if ((unsigned)m->source_language > FIR_LIMBO || (unsigned)m->source_semantics > FIR_LIMBO_TABLES ||
        (!m->source_language != !m->source_semantics) ||
        (m->source_language && (m->source_language == FIR_LIMBO
                                    ? (m->source_semantics != FIR_LIMBO_I32 && m->source_semantics != FIR_LIMBO_REAL &&
                                       m->source_semantics != FIR_LIMBO_TABLES)
                                    : m->source_semantics != FIR_AVM1_NUMERIC)))
    {
        return fail(error, cap, "incompatible source language/semantics profile");
    }
    if (!symbol_name(m->module, '@'))
    {
        return fail(error, cap, "invalid module spelling");
    }
    for (b = 0; b < m->block_count; ++b)
    {
        if (!symbol_name(m->blocks[b].name, '^'))
        {
            return fail(error, cap, "unterminated block name");
        }
        for (j = 0; j < b; ++j)
        {
            if (!strcmp(m->blocks[b].name, m->blocks[j].name))
            {
                return fail(error, cap, "duplicate block spelling %s", m->blocks[b].name);
            }
        }
    }
    for (i = 0; i < m->value_count; ++i)
    {
        int d = m->values[i].definition;
        if (!symbol_name(m->values[i].name, '%') || d < -1 || d >= m->count ||
            (unsigned)m->values[i].type > T_BOOL_SLOT ||
            (d >= 0 && (m->instructions[d].result != i || m->instructions[d].type != m->values[i].type)))
        {
            return fail(error, cap, "invalid value definition table");
        }
        for (j = 0; j < i; ++j)
        {
            if (!strcmp(m->values[i].name, m->values[j].name))
            {
                return fail(error, cap, "duplicate value spelling %s", m->values[i].name);
            }
        }
    }
    for (i = 0; i < m->count; ++i)
    {
        FirInstruction *in = &m->instructions[i];
        int unary, binary;
        if ((unsigned)in->op > O_STRING_LT || (unsigned)in->type > T_BOOL_SLOT || in->block < 0 ||
            in->block >= m->block_count || in->result < -1 || in->result >= m->value_count || in->a < -1 ||
            in->a >= m->value_count || in->b < -1 || in->b >= m->value_count || in->yes < -1 ||
            in->yes >= m->block_count || in->no < -1 || in->no >= m->block_count || !memchr(in->text, 0, FIR_TEXT))
        {
            return fail(error, cap, "invalid instruction fields");
        }
        if (in->removed)
        {
            continue;
        }
        if (in->op == O_STRING && !fir_utf8((const unsigned char *)in->text, strlen(in->text)))
        {
            return fail(error, cap, "invalid UTF-8 string constant");
        }
        if ((in->op == O_BOOL && in->integer != 0 && in->integer != 1) || (in->op == O_F64 && !isfinite(in->real)))
        {
            return fail(error, cap, "invalid constant payload");
        }
        unary = in->op == O_STRING_LEN || in->op == O_BOOL_LOAD || in->op == O_TABLE_COUNT || in->op == O_TABLE_HAS ||
                in->op == O_TABLE_DELETE || in->op == O_COPY || in->op == O_NUMBER || in->op == O_RECEIVE ||
                in->op == O_TRACE || in->op == O_CLOSE || in->op == O_IF || in->op == O_TABLE_LOAD ||
                in->op == O_TABLE_LEN || in->op == O_TABLE_KEYS || in->op == O_KEYS_LEN || in->op == O_VALUE_TYPE ||
                in->op == O_TO_BIG || in->op == O_TO_INT || in->op == O_TO_TEXT || in->op == O_BIG_NEG ||
                in->op == O_BIG_NOT;
        binary = in->op == O_STRING_LT || in->op == O_I32_SHL || in->op == O_I32_SHR || in->op == O_STRING_EQ ||
                 in->op == O_STRING_CONCAT || in->op == O_I32_DIV || in->op == O_I32_MOD || in->op == O_STRING_AT ||
                 in->op == O_STRING_SLICE || in->op == O_BOOL_STORE || (in->op >= O_ADD && in->op <= O_ASADD) ||
                 in->op == O_SEND || in->op == O_TABLE_INDEX || (in->op >= O_BIG_ADD && in->op <= O_BIG_MUL) ||
                 (in->op >= O_BIG_DIV && in->op <= O_BIG_XOR) || in->op == O_BIG_SHL || in->op == O_BIG_SHR ||
                 in->op == O_BIG_LT || in->op == O_BIG_EQ || in->op == O_KEYS_AT || in->op == O_TABLE_STORE ||
                 in->op == O_BIG_RANGE || in->op == O_TABLE_SLICE;
        if ((in->a >= 0) != (unary || binary) || (in->b >= 0) != binary ||
            (in->yes >= 0) != (in->op == O_IF || in->op == O_JUMP) || (in->no >= 0) != (in->op == O_IF) ||
            (in->result >= 0 && m->values[in->result].definition != i))
        {
            return fail(error, cap, "invalid operand/target arity");
        }
    }
    for (b = 0; b < m->block_count; ++b)
    {
        first[b] = last[b] = -1;
        if (!m->blocks[b].defined)
        {
            return fail(error, cap, "undefined block %s", m->blocks[b].name);
        }
    }
    for (i = 0; i < m->count; ++i)
    {
        FirInstruction *in = &m->instructions[i];
        if (in->removed)
        {
            continue;
        }
        b = in->block;
        if (previous < 0 && b != m->instructions[0].block)
        {
            return fail(error, cap, "first live block must be the entry block");
        }
        if (previous != b && first[b] >= 0)
        {
            return fail(error, cap, "interleaved live block layout");
        }
        previous = b;
        if (terminated[b])
        {
            return fail(error, cap, "line %d: instruction after terminator", in->line);
        }
        if (first[b] < 0)
        {
            first[b] = i;
        }
        last[b] = i;
        if (((in->op < O_TRACE || (in->op > O_RETURN && in->op != O_TABLE_STORE && in->op != O_BOOL_STORE))) !=
            (in->result >= 0))
        {
            return fail(error, cap, "line %d: wrong result arity", in->line);
        }
        if (in->op == O_IF || in->op == O_JUMP || in->op == O_RETURN)
        {
            terminated[b] = 1;
            if (in->yes >= 0)
            {
                edge[b][in->yes] = 1;
            }
            if (in->no >= 0)
            {
                edge[b][in->no] = 1;
            }
        }
    }
    entry = m->instructions[0].block;
    reachable[entry] = 1;
    do
    {
        changed = 0;
        for (b = 0; b < m->block_count; ++b)
        {
            if (reachable[b])
            {
                for (j = 0; j < m->block_count; ++j)
                {
                    if (edge[b][j] && !reachable[j])
                    {
                        reachable[j] = 1;
                        changed = 1;
                    }
                }
            }
        }
    } while (changed);
    for (b = 0; b < m->block_count; ++b)
    {
        if (first[b] < 0 || !terminated[b])
        {
            return fail(error, cap, "empty/unterminated block %s", m->blocks[b].name);
        }
        if (!reachable[b])
        {
            return fail(error, cap, "unreachable block %s", m->blocks[b].name);
        }
        for (j = 0; j < m->block_count; ++j)
        {
            dom[b][j] = (unsigned char)(b == entry ? j == entry : 1);
        }
    }
    do
    {
        changed = 0;
        for (b = 0; b < m->block_count; ++b)
        {
            if (b != entry)
            {
                for (j = 0; j < m->block_count; ++j)
                {
                    int value = 1;
                    for (k = 0; k < m->block_count; ++k)
                    {
                        if (edge[k][b])
                        {
                            value &= dom[k][j];
                        }
                    }
                    if (j == b)
                    {
                        value = 1;
                    }
                    if (dom[b][j] != value)
                    {
                        dom[b][j] = (unsigned char)value;
                        changed = 1;
                    }
                }
            }
        }
    } while (changed);
    for (i = 0; i < m->count; ++i)
    {
        FirInstruction *in = &m->instructions[i];
        FirType a = T_VOID, c = T_VOID, expected = T_VOID;
        int operands[2] = {in->a, in->b};
        if (in->removed)
        {
            continue;
        }
        for (j = 0; j < 2; ++j)
        {
            if (operands[j] >= 0)
            {
                int d = m->values[operands[j]].definition;
                if (d < 0 || m->instructions[d].removed || !dom[in->block][m->instructions[d].block] ||
                    (in->block == m->instructions[d].block && d >= i))
                {
                    return fail(error, cap, "line %d: undefined/non-dominating operand", in->line);
                }
            }
        }
        if (in->a >= 0)
        {
            a = m->values[in->a].type;
        }
        if (in->b >= 0)
        {
            c = m->values[in->b].type;
        }
        switch (in->op)
        {
        case O_I64:
            expected = T_I64;
            break;
        case O_TABLE_NEW:
            expected = T_TABLE_BIG;
            break;
        case O_TABLE_INDEX:
            if (a != T_TABLE_BIG || (c != T_I64 && c != T_STRING && c != T_TABLE_KEY))
            {
                goto bad_type;
            }
            expected = T_TABLE_SLOT;
            break;
        case O_TO_INT:
            if (a != T_I64 && a != T_I32 && a != T_BOOL)
            {
                goto bad_type;
            }
            expected = T_I32;
            break;
        case O_BIG_SHL:
        case O_BIG_SHR:
            if (a != T_I64 || c != T_I32)
            {
                goto bad_type;
            }
            expected = T_I64;
            break;
        case O_BIG_LT:
        case O_BIG_EQ:
            if (a != T_I64 || c != T_I64)
            {
                goto bad_type;
            }
            expected = T_BOOL;
            break;
        case O_BIG_ADD:
        case O_BIG_SUB:
        case O_BIG_MUL:
        case O_BIG_NEG:
        case O_BIG_DIV:
        case O_BIG_MOD:
        case O_BIG_AND:
        case O_BIG_OR:
        case O_BIG_XOR:
        case O_BIG_NOT:
            if (a != T_I64 || (in->op != O_BIG_NEG && in->op != O_BIG_NOT && c != T_I64))
            {
                goto bad_type;
            }
            expected = T_I64;
            break;
        case O_TO_BIG:
        case O_TO_TEXT:
            if (a != T_BOOL && a != T_I32 && a != T_I64 && a != T_STRING && a != T_TABLE_KEY)
            {
                goto bad_type;
            }
            expected = in->op == O_TO_BIG ? T_I64 : T_STRING;
            break;
        case O_VALUE_TYPE:
            if (a != T_BOOL && a != T_I32 && a != T_I64 && a != T_STRING && a != T_TABLE_BIG && a != T_KEYS &&
                a != T_TABLE_KEY)
            {
                goto bad_type;
            }
            expected = T_STRING;
            break;
        case O_KEYS_AT:
            if (a != T_KEYS || c != T_I64)
            {
                goto bad_type;
            }
            expected = T_TABLE_KEY;
            break;
        case O_TABLE_LOAD:
            if (a != T_TABLE_SLOT)
            {
                goto bad_type;
            }
            expected = T_I64;
            break;
        case O_TABLE_HAS:
        case O_TABLE_DELETE:
            if (a != T_TABLE_SLOT)
            {
                goto bad_type;
            }
            expected = T_BOOL;
            break;
        case O_STRING_LEN:
            if (a != T_STRING)
            {
                goto bad_type;
            }
            expected = T_I32;
            break;
        case O_STRING_AT:
            if (a != T_STRING || c != T_I32)
            {
                goto bad_type;
            }
            expected = T_I32;
            break;
        case O_STRING_LT:
        case O_STRING_EQ:
            if (a != T_STRING || c != T_STRING)
            {
                goto bad_type;
            }
            expected = T_BOOL;
            break;
        case O_STRING_CONCAT:
            if (a != T_STRING || c != T_STRING)
            {
                goto bad_type;
            }
            expected = T_STRING;
            break;
        case O_STRING_SLICE:
            if (a != T_STRING || c != T_BIG_RANGE)
            {
                goto bad_type;
            }
            expected = T_STRING;
            break;
        case O_BOOL_LOCAL:
            expected = T_BOOL_SLOT;
            break;
        case O_BOOL_LOAD:
            if (a != T_BOOL_SLOT)
            {
                goto bad_type;
            }
            expected = T_BOOL;
            break;
        case O_BOOL_STORE:
            if (a != T_BOOL_SLOT || c != T_BOOL)
            {
                goto bad_type;
            }
            break;
        case O_TABLE_STORE:
            if (a != T_TABLE_SLOT || c != T_I64)
            {
                goto bad_type;
            }
            break;
        case O_TABLE_COUNT:
        case O_TABLE_LEN:
        case O_KEYS_LEN:
            if (a != (in->op == O_KEYS_LEN ? T_KEYS : T_TABLE_BIG))
            {
                goto bad_type;
            }
            expected = T_I64;
            break;
        case O_BIG_RANGE:
            if (a != T_I64 || c != T_I64)
            {
                goto bad_type;
            }
            expected = T_BIG_RANGE;
            break;
        case O_TABLE_SLICE:
            if (a != T_TABLE_BIG || c != T_BIG_RANGE)
            {
                goto bad_type;
            }
            expected = T_TABLE_BIG;
            break;
        case O_TABLE_KEYS:
            if (a != T_TABLE_BIG)
            {
                goto bad_type;
            }
            expected = T_KEYS;
            break;
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
            if (a == T_BOOL_SLOT)
            {
                goto bad_type;
            }
            expected = a;
            break;
        case O_I32_SHL:
        case O_I32_SHR:
        case O_I32_DIV:
        case O_I32_MOD:
            if (a != T_I32 || c != T_I32)
            {
                goto bad_type;
            }
            expected = T_I32;
            break;
        case O_ADD:
        case O_SUB:
        case O_MUL:
        case O_LT:
        case O_EQ:
            if (a != T_I32 || c != T_I32)
            {
                goto bad_type;
            }
            expected = in->op >= O_LT ? T_BOOL : T_I32;
            break;
        case O_FADD:
        case O_FSUB:
        case O_FMUL:
        case O_FDIV:
            if (a != T_F64 || c != T_F64)
            {
                goto bad_type;
            }
            expected = T_F64;
            break;
        case O_ASADD:
            if (!a || !c || a > T_ANY || c > T_ANY)
            {
                goto bad_type;
            }
            expected = T_ANY;
            break;
        case O_NUMBER:
            if (!a || a > T_ANY)
            {
                goto bad_type;
            }
            expected = T_F64;
            break;
        case O_CHANNEL:
            if (!m->channels)
            {
                return fail(error, cap, "channels require requires flashvm.channels");
            }
            expected = T_CHANNEL;
            break;
        case O_RECEIVE:
            if (a != T_CHANNEL || !m->channels)
            {
                goto bad_type;
            }
            expected = T_ANY;
            break;
        case O_SEND:
            if (a != T_CHANNEL || c < T_I32 || c > T_ANY || !m->channels)
            {
                goto bad_type;
            }
            break;
        case O_CLOSE:
            if (a != T_CHANNEL || !m->channels)
            {
                goto bad_type;
            }
            break;
        case O_TRACE:
            if (!a || a == T_CHANNEL || a >= T_TABLE_BIG)
            {
                goto bad_type;
            }
            break;
        case O_IF:
            if (a != T_BOOL)
            {
                goto bad_type;
            }
            break;
        default:
            break;
        }
        if (expected != in->type)
        {
            goto bad_type;
        }
        continue;
    bad_type:
        return fail(error, cap, "line %d: operand/result type mismatch", in->line);
    }
    return 1;
}

/* Verify the borrowed module and perform a bounded topological walk of its live CFG.
 * error/cap receive an optional diagnostic; no IR or parser state is changed and no heap is acquired. */
int fir_acyclic(FirModule *module, char *error, size_t cap)
{
    unsigned char edges[FIR_BLOCKS][FIR_BLOCKS] = {{0}};
    int incoming[FIR_BLOCKS] = {0};
    int queue[FIR_BLOCKS];
    int begin = 0;
    int end = 0;
    int i;
    if (!error)
    {
        cap = 0;
    }
    if (!fir_verify(module, error, cap))
    {
        return 0;
    }
    for (i = 0; i < module->count; ++i)
    {
        const FirInstruction *in = &module->instructions[i];
        int targets[2] = {in->yes, in->no};
        int j;
        if (in->removed)
        {
            continue;
        }
        for (j = 0; j < 2; ++j)
        {
            int target = targets[j];
            if (target >= 0 && !edges[in->block][target])
            {
                edges[in->block][target] = 1;
                incoming[target]++;
            }
        }
    }
    for (i = 0; i < module->block_count; ++i)
    {
        if (!incoming[i])
        {
            queue[end++] = i;
        }
    }
    while (begin < end)
    {
        int block = queue[begin++];
        for (i = 0; i < module->block_count; ++i)
        {
            if (edges[block][i] && --incoming[i] == 0)
            {
                queue[end++] = i;
            }
        }
    }
    return end == module->block_count ? 1 : fail(error, cap, "CFG contains a cycle");
}

/* Escape a decoded string into canonical, reparsable FIR spelling. */
static void quoted(FILE *out, const char *text)
{
    const unsigned char *s = (const unsigned char *)text;
    fputc('"', out);
    for (; *s; ++s)
    {
        if (*s == '\n')
        {
            fputs("\\n", out);
        }
        else if (*s == '\r')
        {
            fputs("\\r", out);
        }
        else if (*s == '\t')
        {
            fputs("\\t", out);
        }
        else
        {
            if (*s == '\\' || *s == '"')
            {
                fputc('\\', out);
            }
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
        fprintf(out,
                "source_language %s\nsource_semantics %s\n",
                languages[m->source_language],
                m->source_semantics == FIR_LIMBO_I32      ? "limbo_i32"
                : m->source_semantics == FIR_LIMBO_TABLES ? "limbo_tables"
                : m->source_semantics == FIR_LIMBO_REAL   ? "limbo_real"
                                                          : "avm1_numeric");
    }
    if (m->channels)
    {
        fputs("requires flashvm.channels\n", out);
    }
    fputs("func @main() -> void {\n", out);
    for (i = 0; i < m->count; ++i)
    {
        const FirInstruction *in = &m->instructions[i];
        if (in->removed)
        {
            continue;
        }
        if (in->block != previous)
        {
            fprintf(out, "%s:\n", m->blocks[in->block].name);
            previous = in->block;
        }
        fputs("    ", out);
        if (in->result >= 0)
        {
            fprintf(out, "%s: %s = ", m->values[in->result].name, types[in->type]);
        }
        fputs(ops[in->op], out);
        if (in->op == O_I64)
        {
            fprintf(out, " %" PRId64, in->big);
        }
        else if (in->op == O_I32)
        {
            fprintf(out, " %ld", (long)in->integer);
        }
        else if (in->op == O_F64)
        {
            fprintf(out, " %.17g", in->real);
        }
        else if (in->op == O_BOOL)
        {
            fprintf(out, " %s", in->integer ? "true" : "false");
        }
        else if (in->op == O_STRING)
        {
            fputc(' ', out);
            quoted(out, in->text);
        }
        else
        {
            if (in->a >= 0)
            {
                fprintf(out, " %s", m->values[in->a].name);
            }
            if (in->b >= 0)
            {
                fprintf(out, ", %s", m->values[in->b].name);
            }
            if (in->yes >= 0)
            {
                fprintf(out, in->a >= 0 ? ", %s" : " %s", m->blocks[in->yes].name);
            }
            if (in->no >= 0)
            {
                fprintf(out, ", %s", m->blocks[in->no].name);
            }
        }
        fprintf(out, " // input line %d\n", in->line);
    }
    fputs("}\n", out);
    return !ferror(out);
}
