/* Scalar hybrid acceptance: holes, exact big values, independent snapshots and OOM rollback. */
#include "table.h"
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>


#ifdef FHT_SANITIZED
/* This restricted host cannot run LSan /proc scans; allocator accounting still covers every block. */
int __lsan_is_turned_off(void)
{
    return 1;
}
#endif


typedef struct
{
    size_t calls; /* Allocation ordinal within one injected-failure run. */
    size_t fail;  /* Fail exactly this ordinal; zero disables injection. */
    size_t live;  /* Outstanding successful allocations. */
} Allocation;

/* Fail one requested allocation while accounting every successful ownership transfer. */
static void *allocate(void *context, size_t size)
{
    Allocation *state = context;
    void *memory;
    state->calls++;
    if (state->calls == state->fail)
    {
        return NULL;
    }
    memory = malloc(size);
    if (memory)
    {
        state->live++;
    }
    return memory;
}

/* Release a tracked block; null is allowed and does not affect live accounting. */
static void release(void *context, void *memory)
{
    Allocation *state = context;
    if (memory)
    {
        state->live--;
        free(memory);
    }
}

/* Construct one borrowed numeric key for tests; negative values exercise rejection. */
static FhtKey index_key(int64_t index)
{
    FhtKey key = {FHT_INDEX, index, NULL, 0};
    return key;
}

/* Construct a borrowed byte-counted key; tests mutate backing storage after insertion. */
static FhtKey text_key(const char *text, size_t size)
{
    FhtKey key = {FHT_TEXT, 0, text, size};
    return key;
}

/* Print a semantic failure and terminate this test process without disabled assertions. */
static void check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL hybrid table: %s\n", message);
        exit(1);
    }
}

/* Check deterministic behavior against a separate fixed numeric reference model. */
static void numeric_model(void)
{
    int present[256] = {0};
    int64_t values[256] = {0};
    uint32_t random = 17;
    unsigned step;
    Fht *table = fht_create(NULL);
    check(table != NULL, "create model table");
    for (step = 0; step < 12000; step++)
    {
        unsigned key;
        unsigned i;
        size_t count = 0;
        uint64_t length = 0;
        int64_t actual = -777;
        random = random * UINT32_C(1664525) + UINT32_C(1013904223);
        key = (random >> 16) & 255;
        if (random & 1)
        {
            values[key] = (int64_t)random * 2000000000LL;
            check(fht_set(table, index_key(key), values[key]) == FHT_OK, "model insert");
            present[key] = 1;
        }
        else
        {
            FhtStatus expected = present[key] ? FHT_OK : FHT_MISSING;
            check(fht_delete(table, index_key(key)) == expected, "model delete");
            present[key] = 0;
        }
        for (i = 0; i < 256; i++)
        {
            FhtStatus result = fht_get(table, index_key(i), &actual);
            if (present[i])
            {
                check(result == FHT_OK && actual == values[i], "model exact value");
                count++;
            }
            else
            {
                check(result == FHT_MISSING, "model absence");
            }
        }
        while (length < 256 && present[length])
        {
            length++;
        }
        check(fht_count(table) == count && fht_length(table) == length, "model geometry");
    }
    fht_destroy(table);
}

/* Exercise independent key ownership, string ordering and snapshots surviving source destruction. */
static void mixed_keys(void)
{
    Fht *table = fht_create(NULL);
    Fht *slice = NULL;
    FhtKeys *keys = NULL;
    int64_t value = 123;
    char name[] = {'k', '\0', 'x'};
    check(table != NULL, "mixed create");
    check(fht_set(table, index_key(INT64_MAX), INT64_MIN) == FHT_OK, "huge sparse key");
    check(fht_set(table, text_key(name, sizeof(name)), INT64_MAX) == FHT_OK, "binary string key");
    name[0] = 'z';
    check(fht_get(table, text_key("k\0x", 3), &value) == FHT_OK && value == INT64_MAX, "copied key");
    check(fht_set(table, text_key("1", 1), 99) == FHT_OK, "numeric-looking string");
    check(fht_set(table, index_key(1), 11) == FHT_OK && fht_length(table) == 0, "hole before one");
    check(fht_set(table, index_key(0), 0) == FHT_OK && fht_length(table) == 2, "stored zero fills hole");
    check(fht_set(table, index_key(2), 22) == FHT_OK, "sequence extend");
    check(fht_slice(table, 1, 3, &slice) == FHT_OK && fht_length(slice) == 2 && fht_count(slice) == 2,
          "slice excludes dictionary and sparse tail");
    check(fht_get(slice, index_key(0), &value) == FHT_OK && value == 11, "slice reindex");
    check(fht_set(slice, index_key(0), 44) == FHT_OK && fht_get(table, index_key(1), &value) == FHT_OK && value == 11,
          "independent slice");
    check(fht_slice(table, 3, 2, &slice) == FHT_INVALID, "backwards slice rejected");
    check(fht_keys(table, &keys) == FHT_OK && keys->count == 6, "all keys snapshot");
    check(keys->items[0].index == 0 && keys->items[3].index == INT64_MAX && keys->items[4].kind == FHT_TEXT &&
              keys->items[4].size == 3 && memcmp(keys->items[4].text, "k\0x", 3) == 0,
          "numeric then insertion order");
    check(fht_delete(table, index_key(0)) == FHT_OK && fht_length(table) == 0, "deletion creates hole");
    check(fht_delete(table, text_key("k\0x", 3)) == FHT_OK, "string unlink");
    check(fht_set(table, index_key(-1), 0) == FHT_INVALID, "negative index rejected");
    value = 123;
    check(fht_get(table, index_key(73), &value) == FHT_MISSING && value == 123, "missing preserves output");
    fht_destroy(slice);
    fht_destroy(table);
    check(keys->items[4].text[0] == 'k', "snapshot survives source retirement");
    fht_keys_destroy(keys);
}

/* Sweep each allocation boundary through writes, snapshots and slices; detect leaks and rollback. */
static void allocation_failures(void)
{
    size_t ordinal;
    for (ordinal = 1; ordinal < 80; ordinal++)
    {
        Allocation state = {0, ordinal, 0};
        FhtAllocator allocator = {&state, allocate, release};
        Fht *table = fht_create(&allocator);
        Fht *slice = NULL;
        FhtKeys *keys = NULL;
        unsigned i;
        if (table)
        {
            for (i = 0; i < 12; i++)
            {
                size_t before = fht_count(table);
                uint64_t length = fht_length(table);
                FhtStatus status = fht_set(table, index_key(i), i);
                if (status == FHT_NOMEM)
                {
                    check(fht_count(table) == before && fht_length(table) == length, "dense rollback");
                }
            }
            for (i = 0; i < 12; i++)
            {
                char text[32];
                size_t before = fht_count(table);
                FhtStatus status;
                snprintf(text, sizeof(text), "string-%u", i);
                status = fht_set(table, text_key(text, strlen(text)), i);
                if (status == FHT_NOMEM)
                {
                    check(fht_count(table) == before, "hash rollback");
                }
            }
            if (fht_keys(table, &keys) == FHT_OK)
            {
                fht_keys_destroy(keys);
            }
            if (fht_slice(table, 0, fht_length(table), &slice) == FHT_OK)
            {
                fht_destroy(slice);
            }
            fht_destroy(table);
        }
        check(state.live == 0, "allocation failure leaked owned storage");
    }
}

/* Verify exact decimal adapters independently of VM ABI and allocator behavior.
 * Outputs remain caller-owned; failed parsing/short formatting must not publish. */
static void scalar_conversions(void)
{
    const int64_t extremes[] = {INT64_MIN, INT64_MAX, 0, -1, 1};
    const char *invalid[] = {"", "+", "-", " 1", "1 ", "1.0", "0x10", "9223372036854775808", "-9223372036854775809"};
    char text[21];
    char oracle[32];
    int64_t result;
    size_t i;
    for (i = 0; i < 20005; i++)
    {
        int64_t value = i < 5 ? extremes[i] : (int64_t)(i * UINT64_C(0x123456789ab));
        if (i >= 5 && i % 2)
        {
            value = -value;
        }
        snprintf(oracle, sizeof(oracle), "%" PRId64, value);
        check(fht_big_format(value, text, sizeof(text)) == strlen(oracle), "exact format length");
        check(strcmp(text, oracle) == 0, "exact format differs from independent decimal oracle");
        result = 7;
        check(fht_big_parse(text, strlen(text), &result) == FHT_OK && result == value, "exact decimal roundtrip");
    }
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
    {
        result = 7;
        check(fht_big_parse(invalid[i], strlen(invalid[i]), &result) == FHT_INVALID && result == 7,
              "invalid parse published a result");
    }
    result = 7;
    check(fht_big_parse("1\0x", 3, &result) == FHT_INVALID && result == 7, "embedded zero accepted");
    check(fht_big_parse("+00042", 6, &result) == FHT_OK && result == 42, "explicit positive decimal");
    text[0] = 'x';
    check(fht_big_format(1, text, 20) == 0 && text[0] == 'x', "short format storage modified");
    check(fht_big_parse(NULL, 1, &result) == FHT_INVALID, "null text accepted");
}

/* Run independent semantic/reference and allocator-failure acceptance suites. */
int main(void)
{
    scalar_conversions();
    numeric_model();
    mixed_keys();
    allocation_failures();
    puts("PASS hybrid big tables: model, holes, snapshots, exact values, slices, OOM ownership");
    return 0;
}
