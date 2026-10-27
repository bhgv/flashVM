/* Independent reference execution for typed table FIR; deliberately not Dis/JIT execution. */
#include "fir_table.h"
#include "table.h"
#include "relative.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


/* Borrow the shared strict validator; no VM representation is used by this oracle. */
extern int fir_utf8(const unsigned char *, size_t);

typedef struct
{
    int64_t big;      /* Exact scalar payload. */
    size_t text_size; /* Exact copied/borrowed byte count, including embedded zeros. */
    char digits[21];  /* Invocation-lifetime exact numeric conversion storage. */
    const char *text; /* Borrowed module/snapshot/static bytes or this value's numeric storage. */
    Fht *table;       /* Borrowed owner from the execution-local registry. */
    FhtKeys *keys;    /* Borrowed independent snapshot from the execution-local registry. */
    FhtKey key;       /* Captured key; text bytes are borrowed from the module or registered snapshot. */
    int64_t first;    /* Range start, validated when applied to a table. */
    int64_t end;      /* Exclusive range end. */
} RuntimeValue;


/* Validate counted runtime UTF-8 while admitting U+0000 as an ordinary scalar.
 * Source text still uses the stricter input validator, which rejects NUL bytes.
 * Split only at ASCII NUL boundaries; malformed multibyte sequences still fail. */
static int runtime_utf8(const char *text, size_t size)
{
    size_t first = 0;
    size_t i;
    if (!size)
    {
        return 1;
    }
    if (!text)
    {
        return 0;
    }
    for (i = 0; i < size; i++)
    {
        if (!text[i])
        {
            if (!fir_utf8((const unsigned char *)text + first, i - first))
            {
                return 0;
            }
            first = i + 1;
        }
    }
    return fir_utf8((const unsigned char *)text + first, size - first);
}


/* Decode borrowed UTF-8 independently of Dis's Rune representation.
 * wanted is a nonnegative scalar position, including the one-past-end position.
 * offset/scalar/length receive byte position, scalar (-1 at end) and total count.
 * Returns false for malformed bytes/bounds/int overflow; allocates nothing. */
static int text_position(const RuntimeValue *value, int64_t wanted, size_t *offset, int64_t *scalar, int64_t *length)
{
    size_t at = 0;
    int64_t count = 0;
    if (wanted < 0 || !runtime_utf8(value->text, value->text_size))
    {
        return 0;
    }
    *offset = 0;
    *scalar = -1;
    while (at < value->text_size)
    {
        unsigned char first = (unsigned char)value->text[at];
        unsigned width = first < 0x80 ? 1U : first < 0xe0 ? 2U : first < 0xf0 ? 3U : 4U;
        uint32_t rune = first & (width == 1 ? 0x7fU : width == 2 ? 0x1fU : width == 3 ? 0x0fU : 0x07U);
        unsigned i;
        if (width > value->text_size - at || count == INT32_MAX)
        {
            return 0;
        }
        for (i = 1; i < width; ++i)
        {
            rune = (rune << 6) | ((unsigned char)value->text[at + i] & 0x3fU);
        }
        if (count == wanted)
        {
            *offset = at;
            *scalar = rune;
        }
        at += width;
        ++count;
    }
    if (wanted == count)
    {
        *offset = value->text_size;
        *scalar = -1;
    }
    *length = count;
    return wanted <= count;
}


/* Interpret low 32 bits as signed int while retaining int64 storage for reference values. */
static int64_t int_bits(uint32_t bits)
{
    return bits <= INT32_MAX ? (int64_t)bits : -1 - (int64_t)(UINT32_MAX - bits);
}


/* Convert wrapped bits to a signed big without relying on implementation-defined
 * out-of-range casts. This scalar helper allocates nothing and never blocks. */
static int64_t big_bits(uint64_t bits)
{
    return bits <= INT64_MAX ? (int64_t)bits : -1 - (int64_t)(UINT64_MAX - bits);
}


/* Check coverage before allocating owners or publishing any trace; module has already verified. */
static int coverage(FirModule *module, char *error, size_t capacity)
{
    int i;
    if (module->channels || !fir_verify(module, error, capacity))
    {
        if (module->channels)
        {
            snprintf(error, capacity, "table reference runner does not implement channel requirements");
        }
        return 0;
    }
    for (i = 0; i < module->count; i++)
    {
        FirInstruction *in = &module->instructions[i];
        if (in->removed)
        {
            continue;
        }
        if ((in->op >= O_I64 && in->op <= O_BIG_STORE) || (in->op >= O_ADD && in->op <= O_EQ) || in->op == O_I32 ||
            in->op == O_BOOL || in->op == O_STRING || in->op == O_COPY || in->op == O_IF || in->op == O_JUMP ||
            in->op == O_RETURN)
        {
            continue;
        }
        if (in->op == O_TRACE && (module->values[in->a].type == T_BOOL || module->values[in->a].type == T_I32 ||
                                  module->values[in->a].type == T_I64 || module->values[in->a].type == T_STRING))
        {
            continue;
        }
        snprintf(error, capacity, "line %d: operation outside table reference runner coverage", in->line);
        return 0;
    }
    return 1;
}


/* Execute only verifier/coverage-admitted instructions. Every allocated owner is registered once,
 * while SSA aliases/locations merely borrow it until this invocation finishes. No C pointer escapes. */
int fir_table_run(FirModule *module, FILE *output, char *error, size_t capacity)
{
    RuntimeValue *values = NULL;
    char **texts = NULL; /* Own concatenations once; RuntimeValue copies/slices only borrow. */
    size_t text_count = 0;
    Fht **tables = NULL;
    FhtKeys **keys = NULL;
    FILE *staged = NULL;
    size_t table_count = 0;
    size_t key_count = 0;
    int i;
    int first[FIR_BLOCKS]; /* First live instruction per verified CFG block. */
    unsigned steps = 0;    /* Bound cyclic reference execution, independently of native VM budgets. */
    int ok = 0;
    char buffer[4096];
    size_t size;
    extern FILE *fir_temporary(void);
    if (!error)
    {
        capacity = 0;
    }
    if (!output)
    {
        snprintf(error, capacity, "table reference runner requires an output stream");
        return 0;
    }
    if (!fir_verify(module, error, capacity) || !coverage(module, error, capacity))
    {
        return 0;
    }
    for (i = 0; i < module->block_count; ++i)
    {
        first[i] = -1;
    }
    for (i = 0; i < module->count; ++i)
    {
        if (!module->instructions[i].removed && first[module->instructions[i].block] < 0)
        {
            first[module->instructions[i].block] = i;
        }
    }
    texts = calloc(FIR_LIMIT, sizeof(*texts));
    values = calloc(FIR_LIMIT, sizeof(*values));
    tables = calloc(FIR_LIMIT, sizeof(*tables));
    keys = calloc(FIR_LIMIT, sizeof(*keys));
    staged = fir_temporary();
    if (!texts || !values || !tables || !keys || !staged)
    {
        snprintf(error, capacity, "table reference runner allocation/staging failure");
        goto done;
    }
    for (i = 0; i < module->count;)
    {
        int next = i + 1;
        if (++steps > 1000000U)
        {
            snprintf(error, capacity, "table reference instruction budget exceeded");
            goto done;
        }
        FirInstruction *in = &module->instructions[i];
        RuntimeValue *result = in->result < 0 ? NULL : &values[in->result];
        RuntimeValue *a = in->a < 0 ? NULL : &values[in->a];
        RuntimeValue *b = in->b < 0 ? NULL : &values[in->b];
        FhtStatus status = FHT_OK;
        if (in->removed)
        {
            i = next;
            continue;
        }
        if (((in->op == O_TABLE_NEW || in->op == O_TABLE_SLICE) && table_count == FIR_LIMIT) ||
            (in->op == O_TABLE_KEYS && key_count == FIR_LIMIT) ||
            ((in->op == O_STRING_CONCAT || in->op == O_RUNE_TEXT || in->op == O_STRING_SPLICE) &&
             text_count == FIR_LIMIT))
        {
            snprintf(error, capacity, "table reference owner budget exceeded");
            goto done;
        }
        switch (in->op)
        {
        case O_ADD:
        case O_SUB:
        case O_MUL:
        case O_I32_DIV:
        case O_I32_MOD:
            if ((in->op == O_I32_DIV || in->op == O_I32_MOD) && !b->big)
            {
                status = FHT_INVALID;
            }
            else
            {
                uint32_t left = (uint32_t)(uint64_t)a->big;
                uint32_t right = (uint32_t)(uint64_t)b->big;
                uint32_t bits = in->op == O_ADD   ? left + right
                                : in->op == O_SUB ? left - right
                                : in->op == O_MUL
                                    ? left * right
                                    : (uint32_t)(uint64_t)(in->op == O_I32_DIV ? a->big / b->big : a->big % b->big);
                result->big = int_bits(bits);
            }
            break;
        case O_LT:
            result->big = a->big < b->big;
            break;
        case O_EQ:
            result->big = a->big == b->big;
            break;
        case O_STRING_LT:
        {
            /* Slices borrow exact byte extents and need not end in NUL.
             * Canonical UTF-8 preserves scalar order; memcmp uses unsigned bytes. */
            size_t common = a->text_size < b->text_size ? a->text_size : b->text_size;
            int compared = memcmp(a->text, b->text, common);
            result->big = compared < 0 || (compared == 0 && a->text_size < b->text_size);
            break;
        }
        case O_STRING_EQ:
            result->big = a->text_size == b->text_size && !memcmp(a->text, b->text, a->text_size);
            break;
        case O_STRING_CONCAT:
        {
            /* Inputs may be non-terminated slices. Copy exact byte extents and
             * register the owner separately so aliases never double-free it. */
            char *text;
            size_t length;
            if (b->text_size == SIZE_MAX || a->text_size > SIZE_MAX - b->text_size - 1)
            {
                status = FHT_NOMEM;
                break;
            }
            length = a->text_size + b->text_size;
            text = malloc(length + 1);
            if (!text)
            {
                status = FHT_NOMEM;
                break;
            }
            memcpy(text, a->text, a->text_size);
            memcpy(text + a->text_size, b->text, b->text_size);
            text[length] = 0;
            texts[text_count++] = text;
            result->text = text;
            result->text_size = length;
            break;
        }
        case O_STRING_LEN:
        case O_STRING_AT:
        {
            size_t offset;
            int64_t scalar;
            int64_t length;
            uint64_t index = 0;
            if (!text_position(a, 0, &offset, &scalar, &length) ||
                (in->op == O_STRING_AT && (!fvm_relative_index((uint64_t)length, b->big, &index) ||
                                           !text_position(a, (int64_t)index, &offset, &scalar, &length) || scalar < 0)))
            {
                status = FHT_INVALID;
            }
            else
            {
                result->big = in->op == O_STRING_LEN ? length : scalar;
            }
            break;
        }
        case O_STRING_SLICE:
        {
            size_t first;
            size_t end;
            int64_t scalar;
            int64_t length;
            uint64_t start_index;
            uint64_t end_index;
            if (!text_position(a, 0, &first, &scalar, &length) ||
                !fvm_relative_range((uint64_t)length, b->first, b->end, &start_index, &end_index) ||
                !text_position(a, (int64_t)start_index, &first, &scalar, &length) ||
                !text_position(a, (int64_t)end_index, &end, &scalar, &length))
            {
                status = FHT_INVALID;
            }
            else
            {
                result->text_size = end - first;
                result->text = result->text_size ? a->text + first : "";
            }
            break;
        }
        case O_BIG_LOCAL:
        case O_BOOL_LOCAL:
            result->big = 0;
            break;
        case O_BIG_STORE:
        case O_BOOL_STORE:
            a->big = b->big;
            break;
        case O_BIG_LOAD:
        case O_BOOL_LOAD:
            result->big = a->big;
            break;
        case O_TO_INT:
        {
            uint32_t bits = (uint32_t)(uint64_t)a->big;
            result->big = bits <= INT32_MAX ? (int64_t)bits : -1 - (int64_t)(UINT32_MAX - bits);
            break;
        }
        case O_I32_SHL:
        case O_I32_SHR:
            if (b->big < 0 || b->big >= 32)
            {
                status = FHT_INVALID;
            }
            else
            {
                unsigned count = (unsigned)b->big;
                uint32_t bits = (uint32_t)(uint64_t)a->big;
                if (in->op == O_I32_SHL)
                {
                    bits <<= count;
                }
                else if (count)
                {
                    bits = (bits >> count) | (a->big < 0 ? UINT32_MAX << (32U - count) : 0);
                }
                result->big = int_bits(bits);
            }
            break;
        case O_BIG_SHL:
        case O_BIG_SHR:
            if (b->big < 0 || b->big >= 64)
            {
                status = FHT_INVALID;
            }
            else
            {
                unsigned count = (unsigned)b->big;
                uint64_t bits = (uint64_t)a->big;
                if (in->op == O_BIG_SHL)
                {
                    bits <<= count;
                }
                else if (count)
                {
                    bits = (bits >> count) | (a->big < 0 ? UINT64_MAX << (64U - count) : 0);
                }
                result->big = big_bits(bits);
            }
            break;
        case O_BIG_LT:
            result->big = a->big < b->big;
            break;
        case O_BIG_EQ:
            result->big = a->big == b->big;
            break;
        case O_I32:
        case O_BOOL:
            result->big = in->integer;
            break;
        case O_BIG_ADD:
            result->big = big_bits((uint64_t)a->big + (uint64_t)b->big);
            break;
        case O_BIG_SUB:
            result->big = big_bits((uint64_t)a->big - (uint64_t)b->big);
            break;
        case O_BIG_MUL:
            result->big = big_bits((uint64_t)a->big * (uint64_t)b->big);
            break;
        case O_BIG_DIV:
        case O_BIG_MOD:
            if (b->big == 0)
            {
                status = FHT_INVALID;
            }
            else if (a->big == INT64_MIN && b->big == -1)
            {
                result->big = in->op == O_BIG_DIV ? INT64_MIN : 0;
            }
            else
            {
                result->big = in->op == O_BIG_DIV ? a->big / b->big : a->big % b->big;
            }
            break;
        case O_BIG_AND:
            result->big = big_bits((uint64_t)a->big & (uint64_t)b->big);
            break;
        case O_BIG_OR:
            result->big = big_bits((uint64_t)a->big | (uint64_t)b->big);
            break;
        case O_BIG_XOR:
            result->big = big_bits((uint64_t)a->big ^ (uint64_t)b->big);
            break;
        case O_BIG_NOT:
            result->big = big_bits(~(uint64_t)a->big);
            break;
        case O_BIG_NEG:
            result->big = big_bits(0U - (uint64_t)a->big);
            break;
        case O_I64:
            result->big = in->big;
            break;
        case O_STRING:
            result->text = in->text;
            result->text_size = strlen(in->text);
            break;
        case O_COPY:
            *result = *a;
            break;
        case O_TABLE_NEW:
            result->table = fht_create(NULL);
            if (!result->table)
            {
                status = FHT_NOMEM;
            }
            else
            {
                tables[table_count++] = result->table;
            }
            break;
        case O_TABLE_INDEX:
            result->table = a->table;
            if (module->values[in->b].type == T_I64)
            {
                result->key.kind = FHT_INDEX;
                result->key.index = b->big;
                status = fht_resolve(a->table, result->key, &result->key);
            }
            else if (module->values[in->b].type == T_TABLE_KEY)
            {
                result->key = b->key;
            }
            else
            {
                result->key.kind = FHT_TEXT;
                result->key.text = b->text;
                result->key.size = b->text_size;
            }
            break;
        /* Sequential oracle has no competing actors; retain operand/type checks
         * through FIR verification, without pretending to test VM scheduling. */
        case O_TABLE_LOCK:
        case O_TABLE_UNLOCK:
            break;
        case O_TABLE_LOAD:
            status = fht_get(a->table, a->key, &result->big);
            break;
        case O_TABLE_STORE:
            status = fht_set(a->table, a->key, b->big);
            break;
        case O_TABLE_HAS:
        {
            int64_t ignored = 0;
            status = fht_get(a->table, a->key, &ignored);
            result->big = status == FHT_OK;
            if (status == FHT_MISSING)
            {
                status = FHT_OK;
            }
            break;
        }
        case O_TABLE_DELETE:
            status = fht_delete(a->table, a->key);
            result->big = status == FHT_OK;
            if (status == FHT_MISSING)
            {
                status = FHT_OK;
            }
            break;
        case O_TABLE_COUNT:
            result->big = (int64_t)fht_count(a->table);
            break;
        case O_TABLE_LEN:
            result->big = (int64_t)fht_length(a->table);
            break;
        case O_BIG_RANGE:
            result->first = a->big;
            result->end = b->big;
            if (a->big >= 0 && b->big >= 0 && b->big < a->big)
            {
                status = FHT_INVALID;
            }
            break;
        case O_RUNE_TEXT:
        {
            uint32_t rune = (uint32_t)(uint64_t)a->big;
            unsigned width;
            unsigned i;
            char *text;
            if (a->big < 0 || rune > 0x10ffffU || (rune >= 0xd800U && rune <= 0xdfffU))
            {
                status = FHT_INVALID;
                break;
            }
            width = rune < 0x80U ? 1U : rune < 0x800U ? 2U : rune < 0x10000U ? 3U : 4U;
            text = malloc(width + 1U);
            if (!text)
            {
                status = FHT_NOMEM;
                break;
            }
            for (i = width; i > 1; i--)
            {
                text[i - 1] = (char)(0x80U | (rune & 0x3fU));
                rune >>= 6;
            }
            text[0] = (char)(rune | (width == 1 ? 0 : width == 2 ? 0xc0U : width == 3 ? 0xe0U : 0xf0U));
            text[width] = 0;
            texts[text_count++] = text;
            result->text = text;
            result->text_size = width;
            break;
        }
        case O_STRING_CHAR_RANGE:
        {
            size_t offset;
            int64_t scalar;
            int64_t length;
            uint64_t index;
            if (!text_position(a, 0, &offset, &scalar, &length) ||
                !fvm_relative_index((uint64_t)length, b->big, &index) || index >= (uint64_t)length)
            {
                status = FHT_INVALID;
                break;
            }
            *result = *a;
            result->first = (int64_t)index;
            result->end = (int64_t)index + 1;
            break;
        }
        case O_STRING_RANGE:
        {
            size_t offset;
            int64_t scalar;
            int64_t length;
            uint64_t first;
            uint64_t end;
            if (!text_position(a, 0, &offset, &scalar, &length) ||
                !fvm_relative_range((uint64_t)length, b->first, b->end, &first, &end))
            {
                status = FHT_INVALID;
                break;
            }
            *result = *a;
            result->first = (int64_t)first;
            result->end = (int64_t)end;
            break;
        }
        case O_STRING_SPLICE:
        {
            size_t first;
            size_t end;
            size_t length;
            int64_t scalar;
            int64_t runes;
            char *text;
            if (!text_position(a, a->first, &first, &scalar, &runes) ||
                !text_position(a, a->end, &end, &scalar, &runes) || first > end)
            {
                status = FHT_INVALID;
                break;
            }
            length = a->text_size - (end - first);
            if (b->text_size == SIZE_MAX || length > SIZE_MAX - b->text_size - 1)
            {
                status = FHT_NOMEM;
                break;
            }
            length += b->text_size;
            text = malloc(length + 1);
            if (!text)
            {
                status = FHT_NOMEM;
                break;
            }
            memcpy(text, a->text, first);
            memcpy(text + first, b->text, b->text_size);
            memcpy(text + first + b->text_size, a->text + end, a->text_size - end);
            text[length] = 0;
            texts[text_count++] = text;
            result->text = text;
            result->text_size = length;
            break;
        }
        case O_TABLE_RANGE:
        {
            uint64_t first;
            uint64_t end;
            if (!fvm_relative_range(fht_length(a->table), b->first, b->end, &first, &end))
            {
                status = FHT_INVALID;
                break;
            }
            result->table = a->table;
            result->first = (int64_t)first;
            result->end = (int64_t)end;
            break;
        }
        case O_TABLE_SPLICE:
            status = fht_splice(a->table, a->first, a->end, b->table);
            break;
        case O_TABLE_SLICE:
            status = fht_slice_relative(a->table, b->first, b->end, &result->table);
            if (status == FHT_OK)
            {
                tables[table_count++] = result->table;
            }
            break;
        case O_TABLE_KEYS:
            status = fht_keys(a->table, &result->keys);
            if (status == FHT_OK)
            {
                keys[key_count++] = result->keys;
            }
            break;
        case O_TO_BIG:
            if (module->values[in->a].type == T_BOOL || module->values[in->a].type == T_I32 ||
                module->values[in->a].type == T_I64)
            {
                result->big = a->big;
            }
            else if (module->values[in->a].type == T_TABLE_KEY && a->key.kind == FHT_INDEX)
            {
                result->big = a->key.index;
            }
            else
            {
                const char *text = module->values[in->a].type == T_STRING ? a->text : a->key.text;
                size_t length = module->values[in->a].type == T_STRING ? a->text_size : a->key.size;
                status = fht_big_parse(text, length, &result->big);
            }
            break;
        case O_TO_TEXT:
            if (module->values[in->a].type == T_STRING)
            {
                result->text = a->text;
                result->text_size = a->text_size;
            }
            else if (module->values[in->a].type == T_TABLE_KEY && a->key.kind == FHT_TEXT)
            {
                result->text = a->key.text;
                result->text_size = a->key.size;
            }
            else
            {
                int64_t number = (module->values[in->a].type == T_BOOL || module->values[in->a].type == T_I64 ||
                                  module->values[in->a].type == T_I32)
                                     ? a->big
                                     : a->key.index;
                result->text_size = fht_big_format(number, result->digits, sizeof(result->digits));
                result->text = result->digits;
            }
            break;
        case O_VALUE_TYPE:
            switch (module->values[in->a].type)
            {
            case T_BOOL:
            case T_I32:
                result->text = "int";
                break;
            case T_I64:
                result->text = "big";
                break;
            case T_STRING:
                result->text = "string";
                break;
            case T_TABLE_BIG:
                result->text = "table of big";
                break;
            case T_KEYS:
                result->text = "keys";
                break;
            default:
                result->text = a->key.kind == FHT_INDEX ? "big" : "string";
                break;
            }
            result->text_size = strlen(result->text);
            break;
        case O_KEYS_AT:
            if (b->big < 0 || (uint64_t)b->big >= a->keys->count)
            {
                status = FHT_INVALID;
            }
            else
            {
                result->key = a->keys->items[b->big];
            }
            break;
        case O_KEYS_LEN:
            result->big = (int64_t)a->keys->count;
            break;
        case O_TRACE:
            if (module->values[in->a].type == T_BOOL || module->values[in->a].type == T_I32 ||
                module->values[in->a].type == T_I64 || module->values[in->a].type == T_BOOL)
            {
                fprintf(staged, "%" PRId64 "\n", a->big);
            }
            else
            {
                fwrite(a->text, 1, a->text_size, staged);
                fputc('\n', staged);
            }
            break;
        case O_IF:
            next = first[a->big ? in->yes : in->no];
            break;
        case O_JUMP:
            next = first[in->yes];
            break;
        case O_RETURN:
            next = module->count;
            break;
        default:
            status = FHT_INVALID; /* Coverage preflight makes this unreachable for admitted input. */
            break;
        }
        if (status != FHT_OK)
        {
            snprintf(error,
                     capacity,
                     "line %d: table operation failed: %s",
                     in->line,
                     (in->op == O_BIG_SHL || in->op == O_BIG_SHR || in->op == O_I32_SHL || in->op == O_I32_SHR)
                         ? "invalid argument"
                     : (in->op == O_BIG_DIV || in->op == O_BIG_MOD || in->op == O_I32_DIV || in->op == O_I32_MOD)
                         ? "zero divide"
                     : in->op == O_TO_BIG    ? "invalid big conversion"
                     : status == FHT_MISSING ? "missing key"
                     : status == FHT_NOMEM   ? "allocation failure"
                                             : "invalid key/range");
            goto done;
        }
        i = next;
    }
    if (ferror(staged) || fflush(staged) || fseek(staged, 0, SEEK_SET))
    {
        snprintf(error, capacity, "table trace staging failure");
        goto done;
    }
    while ((size = fread(buffer, 1, sizeof(buffer), staged)) != 0)
    {
        if (fwrite(buffer, 1, size, output) != size)
        {
            snprintf(error, capacity, "table trace publication failure");
            goto done;
        }
    }
    ok = !ferror(staged) && !ferror(output);
done:
    while (text_count)
    {
        free(texts[--text_count]);
    }
    free(texts);
    while (key_count)
    {
        fht_keys_destroy(keys[--key_count]);
    }
    while (table_count)
    {
        fht_destroy(tables[--table_count]);
    }
    if (staged)
    {
        fclose(staged);
    }
    free(keys);
    free(tables);
    free(values);
    return ok;
}
