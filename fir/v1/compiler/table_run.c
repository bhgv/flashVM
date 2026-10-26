/* Independent reference execution for typed table FIR; deliberately not Dis/JIT execution. */
#include "fir_table.h"
#include "table.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    int64_t big;      /* Exact scalar payload. */
    const char *text; /* Borrowed immutable text in the caller-owned verified module. */
    Fht *table;       /* Borrowed owner from the execution-local registry. */
    FhtKeys *keys;    /* Borrowed independent snapshot from the execution-local registry. */
    FhtKey key;       /* Captured key; any string bytes remain rooted by the source module. */
    int64_t first;    /* Range start, validated when applied to a table. */
    int64_t end;      /* Exclusive range end. */
} RuntimeValue;

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
        if ((in->op >= O_I64 && in->op <= O_KEYS_LEN) || in->op == O_STRING || in->op == O_COPY || in->op == O_RETURN)
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
        case O_I64:
            result->big = in->big;
            break;
        case O_STRING:
            result->text = in->text;
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
            else
            {
                result->key.kind = FHT_TEXT;
                result->key.text = b->text;
                result->key.size = strlen(b->text);
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
                fprintf(staged, "%s\n", a->text);
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
                     status == FHT_MISSING ? "missing key"
                     : status == FHT_NOMEM ? "allocation failure"
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
