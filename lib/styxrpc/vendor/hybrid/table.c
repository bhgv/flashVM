/* Dense scalar slots plus chained hash storage. Keys and values stay at a single owner. */
#include "table.h"
#include "relative.h"
#include <stdlib.h>
#include <string.h>

#define FHT_BUCKETS 16U        /* Initial power-of-two bucket count. */
#define FHT_DENSE_MAX 1048576U /* Dense growth ceiling, not a key/count limit. */

typedef struct
{
    int present;   /* Separates a missing slot from a stored zero. */
    int64_t value; /* Exact Limbo big payload; never converted through double. */
} Slot;

typedef struct Entry
{
    struct Entry *next;       /* Owned bucket chain link. */
    struct Entry *order_next; /* Borrowed string insertion-order link. */
    struct Entry *order_prev; /* Borrowed previous string entry. */
    FhtKey key;               /* Text bytes follow this allocation; numeric keys own no text. */
    int64_t value;            /* Exact copied scalar. */
    uint64_t hash;            /* Cached namespace-aware hash. */
} Entry;

struct Fht
{
    FhtAllocator allocator; /* Copied allocator; context lifetime belongs to caller. */
    Slot *dense;            /* Owned initialized slots. */
    size_t capacity;        /* Dense allocated slot count, not visible sequence length. */
    Entry **buckets;        /* Owned hash directory. */
    size_t bucket_count;    /* Power of two, or zero before the first hash insertion. */
    size_t sparse_count;    /* Hash entries, including string keys. */
    size_t count;           /* All present dense/hash pairs. */
    uint64_t length;        /* First missing nonnegative integer key. */
    Entry *strings_first;   /* Borrowed first string in insertion order. */
    Entry *strings_last;    /* Borrowed last string in insertion order. */
};


/* Default allocator ignores context; returns malloc-owned storage or null. */
static void *default_allocate(void *context, size_t size)
{
    (void)context;
    return malloc(size);
}


/* Default release ignores context and accepts null. */
static void default_release(void *context, void *memory)
{
    (void)context;
    free(memory);
}


/* Allocate and zero a checked array with the borrowed allocator; returns null on overflow/OOM. */
static void *zeroed(FhtAllocator allocator, size_t count, size_t width)
{
    void *memory;
    if (!count || count > SIZE_MAX / width)
    {
        return NULL;
    }
    memory = allocator.allocate(allocator.context, count * width);
    if (memory)
    {
        memset(memory, 0, count * width);
    }
    return memory;
}


/* Validate the borrowed tagged key before hashing or reading its bytes. */
static int valid_key(FhtKey key)
{
    return (key.kind == FHT_INDEX && key.index >= 0) || (key.kind == FHT_TEXT && (key.text || !key.size));
}


/* Compute a namespace-separated FNV hash over the explicit payload; no allocation. */
static uint64_t key_hash(FhtKey key)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    size_t i;
    if (key.kind == FHT_INDEX)
    {
        uint64_t number = (uint64_t)key.index;
        for (i = 0; i < 8; i++)
        {
            hash = (hash ^ (number & 255)) * UINT64_C(1099511628211);
            number >>= 8;
        }
    }
    else
    {
        hash ^= UINT64_C(0xa77);
        for (i = 0; i < key.size; i++)
        {
            hash = (hash ^ (unsigned char)key.text[i]) * UINT64_C(1099511628211);
        }
    }
    return hash;
}


/* Compare validated keys; text pointers are borrowed and zero-length strings need no dereference. */
static int equal_key(FhtKey a, FhtKey b)
{
    if (a.kind != b.kind)
    {
        return 0;
    }
    if (a.kind == FHT_INDEX)
    {
        return a.index == b.index;
    }
    return a.size == b.size && (!a.size || memcmp(a.text, b.text, a.size) == 0);
}


/* Locate the owned bucket link for a validated key; returns null if no directory exists. */
static Entry **find_link(Fht *table, FhtKey key, uint64_t hash)
{
    Entry **link;
    if (!table->bucket_count)
    {
        return NULL;
    }
    link = &table->buckets[hash & (table->bucket_count - 1)];
    while (*link && ((*link)->hash != hash || !equal_key((*link)->key, key)))
    {
        link = &(*link)->next;
    }
    return link;
}


/* Rehash existing entries into a larger initialized directory before publishing it.
 * Returns zero on overflow/OOM, preserving all logical contents and iteration order. */
static int reserve_hash(Fht *table)
{
    size_t count;
    size_t i;
    Entry **buckets;
    if (table->sparse_count < table->bucket_count / 2)
    {
        return 1;
    }
    if (table->bucket_count > SIZE_MAX / 2)
    {
        return 0;
    }
    count = table->bucket_count ? table->bucket_count * 2 : FHT_BUCKETS;
    buckets = zeroed(table->allocator, count, sizeof(*buckets));
    if (!buckets)
    {
        return 0;
    }
    for (i = 0; i < table->bucket_count; i++)
    {
        Entry *entry = table->buckets[i];
        while (entry)
        {
            Entry *next = entry->next;
            size_t bucket = entry->hash & (count - 1);
            entry->next = buckets[bucket];
            buckets[bucket] = entry;
            entry = next;
        }
    }
    table->allocator.release(table->allocator.context, table->buckets);
    table->buckets = buckets;
    table->bucket_count = count;
    return 1;
}


/* Grow a bounded dense region and move any now-covered sparse numeric entries into it.
 * Allocation happens before mutation. Entry count, values and logical length do not change. */
static int reserve_dense(Fht *table, size_t index)
{
    size_t capacity = table->capacity ? table->capacity : 16;
    size_t i;
    Slot *slots;
    while (capacity <= index)
    {
        capacity *= 2;
    }
    slots = zeroed(table->allocator, capacity, sizeof(*slots));
    if (!slots)
    {
        return 0;
    }
    if (table->capacity)
    {
        memcpy(slots, table->dense, table->capacity * sizeof(*slots));
    }
    for (i = 0; i < table->bucket_count; i++)
    {
        Entry **link = &table->buckets[i];
        while (*link)
        {
            Entry *entry = *link;
            if (entry->key.kind == FHT_INDEX && (uint64_t)entry->key.index < capacity)
            {
                slots[entry->key.index].present = 1;
                slots[entry->key.index].value = entry->value;
                *link = entry->next;
                table->sparse_count--;
                table->allocator.release(table->allocator.context, entry);
            }
            else
            {
                link = &entry->next;
            }
        }
    }
    table->allocator.release(table->allocator.context, table->dense);
    table->dense = slots;
    table->capacity = capacity;
    return 1;
}


/* Create an empty scalar owner using a copied allocator; see table.h for lifetime/error contracts. */
Fht *fht_create(const FhtAllocator *allocator)
{
    FhtAllocator selected = {NULL, default_allocate, default_release};
    Fht *table;
    if (allocator)
    {
        selected = *allocator;
    }
    if (!selected.allocate || !selected.release)
    {
        return NULL;
    }
    table = zeroed(selected, 1, sizeof(*table));
    if (table)
    {
        table->allocator = selected;
    }
    return table;
}


/* Release all owned storage at the exclusive table owner; null is accepted. */
void fht_destroy(Fht *table)
{
    FhtAllocator allocator;
    size_t i;
    if (!table)
    {
        return;
    }
    allocator = table->allocator;
    for (i = 0; i < table->bucket_count; i++)
    {
        Entry *entry = table->buckets[i];
        while (entry)
        {
            Entry *next = entry->next;
            allocator.release(allocator.context, entry);
            entry = next;
        }
    }
    allocator.release(allocator.context, table->buckets);
    allocator.release(allocator.context, table->dense);
    allocator.release(allocator.context, table);
}


/* Resolve numeric negatives once against the current dense prefix. Strings and
 * nonnegative sparse keys retain their identity. Borrowed output changes only
 * on success; callers serialize access to the owner. No allocation or blocking. */
FhtStatus fht_resolve(const Fht *table, FhtKey key, FhtKey *result)
{
    uint64_t normalized;
    if (!table || !result)
    {
        return FHT_INVALID;
    }
    if (key.kind == FHT_INDEX && key.index < 0)
    {
        if (!fvm_relative_index(table->length, key.index, &normalized))
        {
            return FHT_INVALID;
        }
        key.index = (int64_t)normalized;
    }
    if (!valid_key(key))
    {
        return FHT_INVALID;
    }
    *result = key;
    return FHT_OK;
}


/* Read a borrowed key without allocation; publish the output only when present. */
FhtStatus fht_get(const Fht *table, FhtKey key, int64_t *value)
{
    Entry **link;
    if (!value || fht_resolve(table, key, &key) != FHT_OK)
    {
        return FHT_INVALID;
    }
    if (key.kind == FHT_INDEX && (uint64_t)key.index < table->capacity)
    {
        Slot slot = table->dense[key.index];
        if (!slot.present)
        {
            return FHT_MISSING;
        }
        *value = slot.value;
        return FHT_OK;
    }
    link = find_link((Fht *)table, key, key_hash(key));
    if (!link || !*link)
    {
        return FHT_MISSING;
    }
    *value = (*link)->value;
    return FHT_OK;
}


/* Copy a scalar/key at the exclusive owner, retaining logical state on allocation failure. */
FhtStatus fht_set(Fht *table, FhtKey key, int64_t value)
{
    Entry **link;
    Entry *entry;
    uint64_t hash;
    int64_t ignored;
    if (fht_resolve(table, key, &key) != FHT_OK)
    {
        return FHT_INVALID;
    }
    if (key.kind == FHT_INDEX && (uint64_t)key.index >= table->capacity && (uint64_t)key.index < FHT_DENSE_MAX &&
        (uint64_t)key.index <= table->capacity * 2 + 16)
    {
        if (!reserve_dense(table, (size_t)key.index))
        {
            return FHT_NOMEM;
        }
    }
    if (key.kind == FHT_INDEX && (uint64_t)key.index < table->capacity)
    {
        Slot *slot = &table->dense[key.index];
        table->count += !slot->present;
        slot->present = 1;
        slot->value = value;
    }
    else
    {
        hash = key_hash(key);
        link = find_link(table, key, hash);
        if (link && *link)
        {
            (*link)->value = value;
            return FHT_OK;
        }
        if (key.kind == FHT_TEXT && key.size > SIZE_MAX - sizeof(*entry))
        {
            return FHT_NOMEM;
        }
        entry =
            table->allocator.allocate(table->allocator.context, sizeof(*entry) + (key.kind == FHT_TEXT ? key.size : 0));
        if (!entry)
        {
            return FHT_NOMEM;
        }
        if (!reserve_hash(table))
        {
            table->allocator.release(table->allocator.context, entry);
            return FHT_NOMEM;
        }
        memset(entry, 0, sizeof(*entry));
        entry->key = key;
        entry->hash = hash;
        entry->value = value;
        if (key.kind == FHT_TEXT)
        {
            entry->key.text = (const char *)(entry + 1);
            if (key.size)
            {
                memcpy((char *)(entry + 1), key.text, key.size);
            }
            entry->order_prev = table->strings_last;
            if (table->strings_last)
            {
                table->strings_last->order_next = entry;
            }
            else
            {
                table->strings_first = entry;
            }
            table->strings_last = entry;
        }
        link = &table->buckets[hash & (table->bucket_count - 1)];
        entry->next = *link;
        *link = entry;
        table->sparse_count++;
        table->count++;
    }
    while (table->length <= INT64_MAX)
    {
        FhtKey next = {FHT_INDEX, (int64_t)table->length, NULL, 0};
        if (fht_get(table, next, &ignored) != FHT_OK)
        {
            break;
        }
        table->length++;
    }
    return FHT_OK;
}


/* Remove a borrowed key and adjust sequence length without shifting other entries. */
FhtStatus fht_delete(Fht *table, FhtKey key)
{
    Entry **link;
    Entry *entry;
    if (fht_resolve(table, key, &key) != FHT_OK)
    {
        return FHT_INVALID;
    }
    if (key.kind == FHT_INDEX && (uint64_t)key.index < table->capacity)
    {
        if (!table->dense[key.index].present)
        {
            return FHT_MISSING;
        }
        table->dense[key.index].present = 0;
    }
    else
    {
        link = find_link(table, key, key_hash(key));
        if (!link || !*link)
        {
            return FHT_MISSING;
        }
        entry = *link;
        *link = entry->next;
        if (key.kind == FHT_TEXT)
        {
            if (entry->order_prev)
            {
                entry->order_prev->order_next = entry->order_next;
            }
            else
            {
                table->strings_first = entry->order_next;
            }
            if (entry->order_next)
            {
                entry->order_next->order_prev = entry->order_prev;
            }
            else
            {
                table->strings_last = entry->order_prev;
            }
        }
        table->sparse_count--;
        table->allocator.release(table->allocator.context, entry);
    }
    table->count--;
    if (key.kind == FHT_INDEX && (uint64_t)key.index < table->length)
    {
        table->length = (uint64_t)key.index;
    }
    return FHT_OK;
}


/* Read cached sequence length from a borrowed owner; null represents zero. */
uint64_t fht_length(const Fht *table)
{
    return table ? table->length : 0;
}


/* Read stored pair count from a borrowed owner; null represents zero. */
size_t fht_count(const Fht *table)
{
    return table ? table->count : 0;
}


/* Compare numeric snapshot keys without subtraction overflow; qsort borrows both operands. */
static int compare_index(const void *left, const void *right)
{
    const FhtKey *a = left;
    const FhtKey *b = right;
    return (a->index > b->index) - (a->index < b->index);
}


/* Build an independently owned all-key snapshot; publish only a complete result. */
FhtStatus fht_keys(const Fht *table, FhtKeys **result)
{
    FhtKeys *snapshot;
    FhtKey *keys;
    size_t i;
    size_t count = 0;
    size_t numeric;
    Entry *entry;
    if (!table || !result)
    {
        return FHT_INVALID;
    }
    snapshot = zeroed(table->allocator, 1, sizeof(*snapshot));
    if (!snapshot)
    {
        return FHT_NOMEM;
    }
    snapshot->allocator = table->allocator;
    keys = zeroed(table->allocator, table->count ? table->count : 1, sizeof(*keys));
    if (!keys)
    {
        fht_keys_destroy(snapshot);
        return FHT_NOMEM;
    }
    snapshot->items = keys;
    for (i = 0; i < table->capacity; i++)
    {
        if (table->dense[i].present)
        {
            keys[count].kind = FHT_INDEX;
            keys[count++].index = (int64_t)i;
        }
    }
    for (i = 0; i < table->bucket_count; i++)
    {
        for (entry = table->buckets[i]; entry; entry = entry->next)
        {
            if (entry->key.kind == FHT_INDEX)
            {
                keys[count++] = entry->key;
            }
        }
    }
    numeric = count;
    qsort(keys, numeric, sizeof(*keys), compare_index);
    snapshot->count = numeric;
    for (entry = table->strings_first; entry; entry = entry->order_next)
    {
        char *text = table->allocator.allocate(table->allocator.context, entry->key.size ? entry->key.size : 1);
        if (!text)
        {
            fht_keys_destroy(snapshot);
            return FHT_NOMEM;
        }
        if (entry->key.size)
        {
            memcpy(text, entry->key.text, entry->key.size);
        }
        keys[count] = entry->key;
        keys[count++].text = text;
        snapshot->count = count;
    }
    *result = snapshot;
    return FHT_OK;
}


/* Retire a key snapshot with its matching allocator; null is accepted. */
void fht_keys_destroy(FhtKeys *keys)
{
    FhtAllocator allocator;
    size_t i;
    if (!keys)
    {
        return;
    }
    allocator = keys->allocator;
    for (i = 0; i < keys->count; i++)
    {
        if (keys->items[i].kind == FHT_TEXT)
        {
            allocator.release(allocator.context, (void *)keys->items[i].text);
        }
    }
    allocator.release(allocator.context, (void *)keys->items);
    allocator.release(allocator.context, keys);
}


/* Copy a checked half-open sequence range into a new independent owner. */
FhtStatus fht_slice(const Fht *table, uint64_t first, uint64_t end, Fht **result)
{
    Fht *copy;
    uint64_t i;
    if (!table || !result || first > end || end > table->length)
    {
        return FHT_INVALID;
    }
    copy = fht_create(&table->allocator);
    if (!copy)
    {
        return FHT_NOMEM;
    }
    for (i = first; i < end; i++)
    {
        FhtKey key = {FHT_INDEX, (int64_t)i, NULL, 0};
        FhtKey target = {FHT_INDEX, (int64_t)(i - first), NULL, 0};
        int64_t value;
        FhtStatus status = fht_get(table, key, &value);
        if (status == FHT_OK)
        {
            status = fht_set(copy, target, value);
        }
        if (status != FHT_OK)
        {
            fht_destroy(copy);
            return status;
        }
    }
    *result = copy;
    return FHT_OK;
}


/* Copy signed/relative bounds using the dense prefix length, omitting dictionary
 * and sparse keys. result is caller-owned on success and untouched on failure. */
FhtStatus fht_slice_relative(const Fht *table, int64_t first, int64_t end, Fht **result)
{
    uint64_t a;
    uint64_t b;
    if (!table || !result || !fvm_relative_range(table->length, first, end, &a, &b))
    {
        return FHT_INVALID;
    }
    return fht_slice(table, a, b, result);
}


/* Build a complete replacement off to the side, including self-source reads.
 * Borrowed table and replacement require exclusive/read ownership respectively.
 * Commit swaps allocator-compatible storage only after every allocation succeeds. */
FhtStatus fht_splice(Fht *table, int64_t first, int64_t end, const Fht *replacement)
{
    uint64_t a;
    uint64_t b;
    uint64_t length;
    uint64_t i;
    size_t k;
    Fht *next;
    FhtKeys *keys = NULL;
    FhtStatus status;
    Fht old;
    int64_t value;
    if (!table || !replacement || !fvm_relative_range(table->length, first, end, &a, &b))
    {
        return FHT_INVALID;
    }
    length = table->length - (b - a);
    if (length > (uint64_t)INT64_MAX || replacement->length > (uint64_t)INT64_MAX - length)
    {
        return FHT_INVALID;
    }
    length += replacement->length;
    status = fht_keys(table, &keys);
    if (status != FHT_OK)
    {
        return status;
    }
    next = fht_create(&table->allocator);
    if (!next)
    {
        fht_keys_destroy(keys);
        return FHT_NOMEM;
    }
    for (k = 0; k < keys->count; k++)
    {
        FhtKey key = keys->items[k];
        if (key.kind == FHT_INDEX && (uint64_t)key.index < table->length)
        {
            continue;
        }
        if (key.kind == FHT_INDEX && (uint64_t)key.index < length)
        {
            status = FHT_INVALID;
            goto done;
        }
        status = fht_get(table, key, &value);
        if (status == FHT_OK)
        {
            status = fht_set(next, key, value);
        }
        if (status != FHT_OK)
        {
            goto done;
        }
    }
    for (i = 0; i < length; i++)
    {
        const Fht *source = table;
        FhtKey source_key = {FHT_INDEX, 0, NULL, 0};
        FhtKey target_key = {FHT_INDEX, (int64_t)i, NULL, 0};
        if (i < a)
        {
            source_key.index = (int64_t)i;
        }
        else if (i - a < replacement->length)
        {
            source = replacement;
            source_key.index = (int64_t)(i - a);
        }
        else
        {
            source_key.index = (int64_t)(i - replacement->length + b - a);
        }
        status = fht_get(source, source_key, &value);
        if (status == FHT_OK)
        {
            status = fht_set(next, target_key, value);
        }
        if (status != FHT_OK)
        {
            goto done;
        }
    }
    old = *table;
    *table = *next;
    *next = old;
    status = FHT_OK;
done:
    fht_keys_destroy(keys);
    fht_destroy(next);
    return status;
}


/* Parse borrowed byte-counted decimal text into an exact signed big. Optional +/-,
 * digits only; no whitespace, prefixes, embedded zero or truncation. On invalid
 * input/overflow output is unchanged. No allocation, blocking or ownership transfer. */
FhtStatus fht_big_parse(const char *text, size_t size, int64_t *result)
{
    uint64_t number = 0;
    uint64_t limit = INT64_MAX;
    size_t position = 0;
    int negative = 0;
    if (!text || !size || !result)
    {
        return FHT_INVALID;
    }
    if (text[0] == '-' || text[0] == '+')
    {
        negative = text[0] == '-';
        position++;
    }
    if (position == size)
    {
        return FHT_INVALID;
    }
    if (negative)
    {
        limit++;
    }
    for (; position < size; position++)
    {
        unsigned digit = (unsigned char)text[position] - (unsigned)'0';
        if (digit > 9 || number > (limit - digit) / 10U)
        {
            return FHT_INVALID;
        }
        number = number * 10U + digit;
    }
    *result = negative ? (number == (uint64_t)INT64_MAX + 1U ? INT64_MIN : -(int64_t)number) : (int64_t)number;
    return FHT_OK;
}


/* Format an exact big into borrowed storage of at least 21 bytes, including NUL.
 * Returns zero for insufficient/null storage without writing. Otherwise returns
 * byte length excluding NUL. No allocation, blocking or native VM dependency. */
size_t fht_big_format(int64_t value, char *text, size_t capacity)
{
    char reversed[20];
    uint64_t magnitude;
    size_t digits = 0;
    size_t size = 0;
    if (!text || capacity < 21)
    {
        return 0;
    }
    magnitude = value < 0 ? (uint64_t)(-(value + 1)) + 1U : (uint64_t)value;
    do
    {
        reversed[digits++] = (char)('0' + magnitude % 10U);
        magnitude /= 10U;
    } while (magnitude);
    if (value < 0)
    {
        text[size++] = '-';
    }
    while (digits)
    {
        text[size++] = reversed[--digits];
    }
    text[size] = 0;
    return size;
}
