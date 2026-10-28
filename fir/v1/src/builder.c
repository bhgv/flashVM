/* Coco/R semantic builder and input validation. The generated scanner/parser
 * has process-global state: serialize parsing; sequential calls reset state.
 * Verification/optimisation/emission use caller-owned modules independently. */
#include "builder.h"
#include "fir1s.h"
#include "fir1p.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <inttypes.h>


extern int fir_utf8(const unsigned char *, size_t);
#define INPUT_LIMIT (2u * 1024u * 1024u)


typedef struct
{
    FirModule *module;
    FirInstruction instruction;
    int block, failed;
    char *error;
    size_t capacity;
} Builder;

static Builder *active;
extern Error_Func Custom_Error;


extern int fir_parser_run(unsigned char *, int);


/* Keep the first semantic failure; later recovery actions cannot overwrite it. */
void fir_build_error(const char *message, int line)
{
    if (!active->failed)
    {
        snprintf(active->error, active->capacity, "line %d: %s", line, message);
    }
    active->failed = 1;
}


/* Translate the generated parser callback into the same bounded diagnostic. */
static void syntax_error(int code, int line, int column, long offset)
{
    (void)column;
    (void)offset;
    if (!active->failed)
    {
        snprintf(active->error, active->capacity, "line %d: FIR syntax error E%d", line, code);
    }
    active->failed = 1;
}


/* Insert forward references now; the verifier later requires definitions. */
static int symbol(const char *text, int block)
{
    FirModule *m = active->module;
    int i, n = block ? m->block_count : m->value_count;
    for (i = 0; i < n; ++i)
    {
        if (!strcmp(text, block ? m->blocks[i].name : m->values[i].name))
        {
            return i;
        }
    }
    if (n >= (block ? FIR_BLOCKS : FIR_LIMIT))
    {
        fir_build_error("symbol limit exceeded", active->instruction.line);
        return -1;
    }
    if (block)
    {
        strcpy(m->blocks[n].name, text);
        ++m->block_count;
    }
    else
    {
        strcpy(m->values[n].name, text);
        m->values[n].definition = -1;
        ++m->value_count;
    }
    return n;
}


/* Reset the next instruction before any of its semantic actions execute. */
void fir_build_begin(int line)
{
    FirInstruction *in = &active->instruction;
    memset(in, 0, sizeof(*in));
    in->result = in->a = in->b = in->yes = in->no = -1;
    in->block = active->block;
    in->line = line;
}


/* Commit a complete instruction only while semantic construction is valid. */
void fir_build_end(void)
{
    FirModule *m = active->module;
    if (active->failed)
    {
        return;
    }
    if (m->count >= FIR_LIMIT)
    {
        fir_build_error("instruction limit exceeded", active->instruction.line);
        return;
    }
    m->instructions[m->count++] = active->instruction;
}


/* Record the exact opcode selected by the grammar alternative. */
void fir_build_op(FirOp op)
{
    active->instruction.op = op;
}


/* Record the explicit requirement for the existing FlashVM channel extension. */
void fir_build_channels(void)
{
    active->module->channels = 1;
}


/* Decimal parsing is independent of host long width. String escapes are
 * decoded after Coco/R has recognised a complete, bounded quoted token. */
static void literal(const char *text, int line)
{
    FirInstruction *in = &active->instruction;
    char *end;
    errno = 0;
    if (in->op == O_STRING || in->op == O_AS_GLOBAL_GET || in->op == O_AS_GLOBAL_SET || in->op == O_AS_MEMBER_SET)
    {
        size_t i, n = 0, size = strlen(text);
        for (i = 1; i + 1 < size; ++i)
        {
            char c = text[i];
            if (c == '\\')
            {
                c = text[++i];
                if (c == 'n')
                {
                    c = '\n';
                }
                else if (c == 'r')
                {
                    c = '\r';
                }
                else if (c == 't')
                {
                    c = '\t';
                }
            }
            in->text[n++] = c;
        }
        in->text[n] = 0;
    }
    else if (in->op == O_BOOL)
    {
        in->integer = !strcmp(text, "true");
    }
    else if (in->op == O_I64)
    {
        intmax_t value = strtoimax(text, &end, 10);
        if (*end || errno || value < INT64_MIN || value > INT64_MAX)
        {
            fir_build_error("invalid i64 literal", line);
        }
        else
        {
            in->big = (int64_t)value;
        }
    }
    else if (in->op == O_I32)
    {
        const char *digits = text + (*text == '-' || *text == '+');
        int base = digits[0] == '0' && (digits[1] == 'x' || digits[1] == 'X') ? 16 : 10;
        long value = strtol(text, &end, base);
        if (*end || errno || value < INT32_MIN || value > INT32_MAX)
        {
            fir_build_error("invalid i32 literal", line);
        }
        else
        {
            in->integer = (int32_t)value;
        }
    }
    else
    {
        in->real = strtod(text, &end);
        /* strtod may report ERANGE for a representable nonzero subnormal.
         * Keep rejecting overflow and values rounded entirely to zero. */
        if (*end || (errno && !(errno == ERANGE && in->real != 0.0 && in->real >= -DBL_MIN && in->real <= DBL_MIN)) ||
            !isfinite(in->real))
        {
            fir_build_error("expected finite f64 literal", line);
        }
    }
}


/* Dispatch a captured grammar token by its semantic role. */
void fir_build_token(int role, const char *text, int line)
{
    FirModule *m = active->module;
    FirInstruction *in = &active->instruction;
    static const char *const types[] = {
        "void",      "i32",         "f64",        "bool",      "string",    "any",       "channel",     "i64",
        "table.big", "table.slot",  "big.range",  "keys",      "table.key", "bool.slot", "table.range", "string.range",
        "i64.slot",  "string.cell", "table.cell", "keys.cell", "key.cell",  "i32.slot",  "f64.slot"};
    int index;
    if (active->failed)
    {
        return;
    }
    if (role == B_VERSION)
    {
        if (strcmp(text, "1.0"))
        {
            fir_build_error("unsupported FIR version", line);
        }
        return;
    }
    if (role == B_FUNCTION)
    {
        if (strcmp(text, "@main"))
        {
            fir_build_error("only @main is supported in FIR 1.0", line);
        }
        return;
    }
    if (role == B_MODULE)
    {
        strcpy(m->module, text);
        return;
    }
    if (role == B_LANGUAGE)
    {
        m->source_language = !strcmp(text, "as1")   ? FIR_AS1
                             : !strcmp(text, "as2") ? FIR_AS2
                             : !strcmp(text, "as3") ? FIR_AS3
                                                    : FIR_LIMBO;
        return;
    }
    if (role == B_SEMANTICS)
    {
        m->source_semantics = !strcmp(text, "avm1_numeric")   ? FIR_AVM1_NUMERIC
                              : !strcmp(text, "limbo_tables") ? FIR_LIMBO_TABLES
                              : !strcmp(text, "limbo_real")   ? FIR_LIMBO_REAL
                                                              : FIR_LIMBO_I32;
        return;
    }
    if (role == B_LITERAL)
    {
        literal(text, line);
        return;
    }
    if (role == B_TYPE)
    {
        for (index = T_I32; index <= T_F64_SLOT; ++index)
        {
            if (!strcmp(types[index], text))
            {
                break;
            }
        }
        if (index > T_F64_SLOT || in->result < 0)
        {
            fir_build_error("invalid type", line);
            return;
        }
        in->type = m->values[in->result].type = (FirType)index;
        return;
    }
    index = symbol(text, role == B_BLOCK || role == B_YES || role == B_NO);
    if (index < 0)
    {
        return;
    }
    switch (role)
    {
    case B_RESULT:
        if (m->values[index].definition >= 0)
        {
            fir_build_error("duplicate SSA definition", line);
        }
        else
        {
            in->result = index;
            m->values[index].definition = m->count;
        }
        break;
    case B_BLOCK:
        if (m->blocks[index].defined)
        {
            fir_build_error("duplicate block", line);
        }
        else
        {
            m->blocks[index].defined = 1;
            active->block = index;
        }
        break;
    case B_A:
        in->a = index;
        break;
    case B_B:
        in->b = index;
        break;
    case B_YES:
        in->yes = index;
        break;
    case B_NO:
        in->no = index;
        break;
    default:
        fir_build_error("invalid builder role", line);
        break;
    }
}


/* Validate the physical FIR version line, optionally followed by a // comment.
 * data/size borrow already validated UTF-8 bytes. Return one only for an exact
 * version plus LF/CRLF; bare trailing spaces, BOM and preceding lines fail.
 * No allocation, parser state or caller storage is acquired or changed. */
static int valid_header(const unsigned char *data, size_t size)
{
    size_t cursor = 7;
    if (size < 8 || memcmp(data, "FIR 1.0", 7))
    {
        return 0;
    }
    if (data[cursor] == '\n' || (data[cursor] == '\r' && size > 8 && data[cursor + 1] == '\n'))
    {
        return 1;
    }
    while (cursor < size && (data[cursor] == ' ' || data[cursor] == '\t'))
    {
        cursor++;
    }
    if (cursor == 7 || size - cursor < 2 || data[cursor] != '/' || data[cursor + 1] != '/')
    {
        return 0;
    }
    cursor += 2;
    while (cursor < size && data[cursor] != '\r' && data[cursor] != '\n')
    {
        cursor++;
    }
    return cursor < size &&
           (data[cursor] == '\n' || (data[cursor] == '\r' && size - cursor > 1 && data[cursor + 1] == '\n'));
}


/* Read and validate bounded input before invoking the generated Coco/R parser. */
int fir_parse(FILE *input, FirModule *m, char *error, size_t capacity)
{
    Builder builder;
    unsigned char *text = (unsigned char *)malloc(INPUT_LIMIT + 1);
    size_t length;
    int ok;
    if (!text)
    {
        snprintf(error, capacity, "out of memory");
        return 0;
    }
    length = fread(text, 1, INPUT_LIMIT + 1, input);
    if (ferror(input) || length > INPUT_LIMIT || !fir_utf8(text, length))
    {
        snprintf(error, capacity, "read error, invalid UTF-8/NUL or input exceeds 2 MiB");
        free(text);
        return 0;
    }
    if (!valid_header(text, length))
    {
        snprintf(error, capacity, "first line must be FIR 1.0");
        free(text);
        return 0;
    }
    memset(m, 0, sizeof(*m));
    memset(&builder, 0, sizeof(builder));
    builder.module = m;
    builder.block = -1;
    builder.error = error;
    builder.capacity = capacity;
    active = &builder;
    Custom_Error = syntax_error;
    ok = fir_parser_run(text, (int)length);
    if (!ok)
    {
        snprintf(error, capacity, "scanner allocation failure");
    }
    ok = ok && Successful() && !builder.failed;
    Custom_Error = 0;
    active = 0;
    free(text);
    return ok;
}
