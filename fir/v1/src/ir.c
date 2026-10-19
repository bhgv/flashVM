/* FIR 1.0 SSA verifier and canonical printer. Parsing lives in the
 * Coco/R grammar compiler/fir1.atg and its semantic builder. */
#include "fir.h"
#include <string.h>
#include <stdarg.h>
/* Format a bounded verifier diagnostic and return failure. */
static int fail(char *error, size_t cap, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(error, cap, format, args);
    va_end(args);
    return 0;
}
static const char *const types[] = {"void", "i32", "f64", "bool", "string", "any", "channel"};
static const char *const ops[] = {
    "const.i32", "const.f64", "const.bool", "const.string", "copy",
    "i32.add.wrap", "i32.sub.wrap", "i32.mul.wrap", "i32.lt.signed", "i32.eq",
    "f64.add", "f64.sub", "f64.mul", "as.add", "as.to_number",
    "limbo.chan_create", "limbo.chan_receive", "trace", "limbo.chan_send", "limbo.chan_close", "if", "jump", "return"
};
/* All blocks must be reachable and terminated. Requiring reachability keeps
 * the initial verifier small and makes dominance precise even for cycles. */
/* Check CFG structure, compute dominators, then validate each SSA operand/type. */
int fir_verify(FirModule *m, char *error, size_t cap)
{
    unsigned char edge[FIR_BLOCKS][FIR_BLOCKS] = {{0}};
    unsigned char dom[FIR_BLOCKS][FIR_BLOCKS], reachable[FIR_BLOCKS] = {0};
    int first[FIR_BLOCKS], last[FIR_BLOCKS], terminated[FIR_BLOCKS] = {0};
    int i, b, j, k, changed, entry;
    if (!m->count || !m->block_count) return fail(error, cap, "empty function");
    for (b = 0; b < m->block_count; ++b) {
        first[b] = last[b] = -1;
        if (!m->blocks[b].defined) return fail(error, cap, "undefined block %s", m->blocks[b].name);
    }
    for (i = 0; i < m->count; ++i) {
        FirInstruction *in = &m->instructions[i];
        if (in->removed) continue;
        b = in->block;
        if (terminated[b]) return fail(error, cap, "line %d: instruction after terminator", in->line);
        if (first[b] < 0) first[b] = i;
        last[b] = i;
        if ((in->op < O_TRACE) != (in->result >= 0)) return fail(error, cap, "line %d: wrong result arity", in->line);
        if (in->op >= O_IF) {
            terminated[b] = 1;
            if (in->yes >= 0) edge[b][in->yes] = 1;
            if (in->no >= 0) edge[b][in->no] = 1;
        }
    }
    entry = m->instructions[0].block;
    reachable[entry] = 1;
    do {
        changed = 0;
        for (b = 0; b < m->block_count; ++b) if (reachable[b])
            for (j = 0; j < m->block_count; ++j) if (edge[b][j] && !reachable[j]) {
                reachable[j] = 1; changed = 1;
            }
    } while (changed);
    for (b = 0; b < m->block_count; ++b) {
        if (first[b] < 0 || !terminated[b]) return fail(error, cap, "empty/unterminated block %s", m->blocks[b].name);
        if (!reachable[b]) return fail(error, cap, "unreachable block %s", m->blocks[b].name);
        for (j = 0; j < m->block_count; ++j) dom[b][j] = (unsigned char)(b == entry ? j == entry : 1);
    }
    do {
        changed = 0;
        for (b = 0; b < m->block_count; ++b) if (b != entry)
            for (j = 0; j < m->block_count; ++j) {
                int value = 1;
                for (k = 0; k < m->block_count; ++k) if (edge[k][b]) value &= dom[k][j];
                if (j == b) value = 1;
                if (dom[b][j] != value) { dom[b][j] = (unsigned char)value; changed = 1; }
            }
    } while (changed);
    for (i = 0; i < m->count; ++i) {
        FirInstruction *in = &m->instructions[i];
        FirType a = T_VOID, c = T_VOID, expected = T_VOID;
        int operands[2] = {in->a, in->b};
        if (in->removed) continue;
        for (j = 0; j < 2; ++j) if (operands[j] >= 0) {
            int d = m->values[operands[j]].definition;
            if (d < 0 || m->instructions[d].removed ||
                !dom[in->block][m->instructions[d].block] ||
                (in->block == m->instructions[d].block && d >= i))
                return fail(error, cap, "line %d: undefined/non-dominating operand", in->line);
        }
        if (in->a >= 0) a = m->values[in->a].type;
        if (in->b >= 0) c = m->values[in->b].type;
        switch (in->op) {
        case O_I32: expected = T_I32; break;
        case O_F64: expected = T_F64; break;
        case O_BOOL: expected = T_BOOL; break;
        case O_STRING: expected = T_STRING; break;
        case O_COPY: expected = a; break;
        case O_ADD: case O_SUB: case O_MUL: case O_LT: case O_EQ:
            if (a != T_I32 || c != T_I32) goto bad_type;
            expected = in->op >= O_LT ? T_BOOL : T_I32; break;
        case O_FADD: case O_FSUB: case O_FMUL:
            if (a != T_F64 || c != T_F64) goto bad_type;
            expected = T_F64; break;
        case O_ASADD:
            if (!a || !c || a == T_CHANNEL || c == T_CHANNEL) goto bad_type;
            expected = T_ANY; break;
        case O_NUMBER:
            if (!a || a == T_CHANNEL) goto bad_type;
            expected = T_F64; break;
        case O_CHANNEL:
            if (!m->channels) return fail(error, cap, "channels require requires flashvm.channels");
            expected = T_CHANNEL; break;
        case O_RECEIVE:
            if (a != T_CHANNEL || !m->channels) goto bad_type;
            expected = T_ANY; break;
        case O_SEND:
            if (a != T_CHANNEL || c < T_I32 || c > T_ANY || !m->channels) goto bad_type;
            break;
        case O_CLOSE:
            if (a != T_CHANNEL || !m->channels) goto bad_type;
            break;
        case O_TRACE: if (!a || a == T_CHANNEL) goto bad_type; break;
        case O_IF: if (a != T_BOOL) goto bad_type; break;
        default: break;
        }
        if (expected != in->type) goto bad_type;
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
    for (; *s; ++s) {
        if (*s == '\n') fputs("\\n", out);
        else if (*s == '\r') fputs("\\r", out);
        else if (*s == '\t') fputs("\\t", out);
        else { if (*s == '\\' || *s == '"') fputc('\\', out); fputc(*s, out); }
    }
    fputc('"', out);
}
/* Emit live instructions in stable input order, retaining original line notes. */
int fir_print(FILE *out, const FirModule *m)
{
    int i, previous = -1;
    fprintf(out, "FIR 1.0\nmodule %s\n", m->module);
    if (m->channels) fputs("requires flashvm.channels\n", out);
    fputs("func @main() -> void {\n", out);
    for (i = 0; i < m->count; ++i) {
        const FirInstruction *in = &m->instructions[i];
        if (in->removed) continue;
        if (in->block != previous) {
            fprintf(out, "%s:\n", m->blocks[in->block].name); previous = in->block;
        }
        fputs("    ", out);
        if (in->result >= 0) fprintf(out, "%s: %s = ", m->values[in->result].name, types[in->type]);
        fputs(ops[in->op], out);
        if (in->op == O_I32) fprintf(out, " %ld", (long)in->integer);
        else if (in->op == O_F64) fprintf(out, " %.17g", in->real);
        else if (in->op == O_BOOL) fprintf(out, " %s", in->integer ? "true" : "false");
        else if (in->op == O_STRING) { fputc(' ', out); quoted(out, in->text); }
        else {
            if (in->a >= 0) fprintf(out, " %s", m->values[in->a].name);
            if (in->b >= 0) fprintf(out, ", %s", m->values[in->b].name);
            if (in->yes >= 0) fprintf(out, in->a >= 0 ? ", %s" : " %s", m->blocks[in->yes].name);
            if (in->no >= 0) fprintf(out, ", %s", m->blocks[in->no].name);
        }
        fprintf(out, " // input line %d\n", in->line);
    }
    fputs("}\n", out);
    return !ferror(out);
}
