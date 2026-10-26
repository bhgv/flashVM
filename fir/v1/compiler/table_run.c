/* Independent reference execution for typed table FIR; deliberately not Dis/JIT execution. */
#include "fir_table.h"
#include "table.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

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
    if (module->block_count != 1 || module->channels)
    {
        snprintf(error, capacity, "table reference runner requires one block without channels");
        return 0;
    }
    for (i = 0; i < module->count; i++)
    {
        FirInstruction *in = &module->instructions[i];
        if (in->removed)
        {
            continue;
        }
        if ((in->op >= O_I64 && in->op <= O_BIG_NOT) || in->op == O_STRING || in->op == O_COPY || in->op == O_RETURN)
        {
            continue;
        }
        if (in->op == O_TRACE && (module->values[in->a].type == T_I64 || module->values[in->a].type == T_STRING))
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
    Fht **tables = NULL;
    FhtKeys **keys = NULL;
    FILE *staged = NULL;
    size_t table_count = 0;
    size_t key_count = 0;
    int i;
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
    values = calloc(FIR_LIMIT, sizeof(*values));
    tables = calloc(FIR_LIMIT, sizeof(*tables));
    keys = calloc(FIR_LIMIT, sizeof(*keys));
    staged = fir_temporary();
    if (!values || !tables || !keys || !staged)
    {
        snprintf(error, capacity, "table reference runner allocation/staging failure");
        goto done;
    }
    for (i = 0; i < module->count; i++)
    {
        FirInstruction *in = &module->instructions[i];
        RuntimeValue *result = in->result < 0 ? NULL : &values[in->result];
        RuntimeValue *a = in->a < 0 ? NULL : &values[in->a];
        RuntimeValue *b = in->b < 0 ? NULL : &values[in->b];
        FhtStatus status = FHT_OK;
        if (in->removed)
        {
            continue;
        }
        switch (in->op)
        {
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
                if (b->big < 0)
                {
                    status = FHT_INVALID;
                }
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
        case O_TABLE_LOAD:
            status = fht_get(a->table, a->key, &result->big);
            break;
        case O_TABLE_STORE:
            status = fht_set(a->table, a->key, b->big);
            break;
        case O_TABLE_LEN:
            result->big = (int64_t)fht_length(a->table);
            break;
        case O_BIG_RANGE:
            result->first = a->big;
            result->end = b->big;
            if (a->big < 0 || b->big < a->big)
            {
                status = FHT_INVALID;
            }
            break;
        case O_TABLE_SLICE:
            status = fht_slice(a->table, (uint64_t)b->first, (uint64_t)b->end, &result->table);
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
            if (module->values[in->a].type == T_I64)
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
                int64_t number = module->values[in->a].type == T_I64 ? a->big : a->key.index;
                result->text_size = fht_big_format(number, result->digits, sizeof(result->digits));
                result->text = result->digits;
            }
            break;
        case O_VALUE_TYPE:
            switch (module->values[in->a].type)
            {
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
            if (module->values[in->a].type == T_I64)
            {
                fprintf(staged, "%" PRId64 "\n", a->big);
            }
            else
            {
                fwrite(a->text, 1, a->text_size, staged);
                fputc('\n', staged);
            }
            break;
        case O_RETURN:
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
                     (in->op == O_BIG_DIV || in->op == O_BIG_MOD) ? "zero divide"
                     : in->op == O_TO_BIG                         ? "invalid big conversion"
                     : status == FHT_MISSING                      ? "missing key"
                     : status == FHT_NOMEM                        ? "allocation failure"
                                                                  : "invalid key/range");
            goto done;
        }
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
