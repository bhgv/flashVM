/* Owner-local C99 hybrid tables. No VM headers, native references or network authority. */
#ifndef FVM_HYBRID_TABLE_H
#define FVM_HYBRID_TABLE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct Fht Fht; /* Opaque mutable table; serialize all access at its VM owner. */

typedef enum
{
    FHT_OK,      /* Operation completed; a stored zero is a valid value. */
    FHT_MISSING, /* Key absent; output value remains unchanged. */
    FHT_INVALID, /* Invalid key, arguments or slice bounds. */
    FHT_NOMEM    /* Allocation failed; logical contents remain unchanged. */
} FhtStatus;

typedef enum
{
    FHT_INDEX, /* Nonnegative signed-64 numeric key. */
    FHT_TEXT   /* Byte-counted string; embedded zero bytes are supported. */
} FhtKeyKind;

typedef struct
{
    FhtKeyKind kind;  /* Numeric or string namespace; 1 and "1" are distinct. */
    int64_t index;    /* FHT_INDEX payload; negatives are invalid. */
    const char *text; /* Borrowed FHT_TEXT bytes; may be null only for empty text. */
    size_t size;      /* Exact text length; never inferred with strlen. */
} FhtKey;

typedef struct
{
    void *context;                     /* Borrowed until table/snapshots are destroyed. */
    void *(*allocate)(void *, size_t); /* Return owned uninitialized storage or null. */
    void (*release)(void *, void *);   /* Release matching storage; must accept null. */
} FhtAllocator;

typedef struct
{
    size_t count;           /* All keys, including sparse numeric and string keys. */
    const FhtKey *items;    /* Immutable copied keys; valid until fht_keys_destroy. */
    FhtAllocator allocator; /* Private matching allocator; callers must not modify. */
} FhtKeys;

/* Create an empty big-valued table. allocator is copied, or null selects malloc/free.
 * Returns null on invalid allocator/OOM. The caller owns the result; no locks or callbacks to VMs. */
Fht *fht_create(const FhtAllocator *allocator);


/* Release a table and all copied keys; null is allowed. Requires exclusive owner access. */
void fht_destroy(Fht *table);


/* Read one key into value only on success. Arguments are borrowed; never allocates or blocks. */
FhtStatus fht_get(const Fht *table, FhtKey key, int64_t *value);


/* Insert/replace a scalar and copy a new text key. Borrowed inputs remain caller-owned.
 * On OOM contents/count/length stay unchanged; internal capacity may have grown. */
FhtStatus fht_set(Fht *table, FhtKey key, int64_t value);


/* Delete an existing key without allocation. Missing/invalid keys leave the table unchanged. */
FhtStatus fht_delete(Fht *table, FhtKey key);


/* Return the first absent nonnegative index, or zero for null. String/sparse keys do not add length. */
uint64_t fht_length(const Fht *table);


/* Return the number of stored pairs, or zero for null; distinct from sequence length. */
size_t fht_count(const Fht *table);


/* Copy all keys: numeric ascending, then strings in insertion order. No table storage escapes.
 * result is assigned only on success. Destroy the snapshot independently of the source table. */
FhtStatus fht_keys(const Fht *table, FhtKeys **result);


/* Release an independently owned key snapshot; null is allowed. */
void fht_keys_destroy(FhtKeys *keys);


/* Copy [first,end) within sequence length into a new zero-based table with the same allocator.
 * Strings/sparse keys are omitted; result is assigned only on success. No source mutation occurs. */
FhtStatus fht_slice(const Fht *table, uint64_t first, uint64_t end, Fht **result);

/* Parse exact signed decimal text. Borrowed byte span/output; output is unchanged
 * on invalid syntax/overflow. Optional +/-, no whitespace/prefixes; never allocates. */
FhtStatus fht_big_parse(const char *text, size_t size, int64_t *result);


/* Format canonical decimal in borrowed storage (capacity >=21), returning bytes
 * excluding NUL. Zero means invalid storage and writes nothing; never allocates. */
size_t fht_big_format(int64_t value, char *text, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif
