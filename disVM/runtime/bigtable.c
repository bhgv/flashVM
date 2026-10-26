/* Native big-table boundary. Only exact scalar payloads and copied UTF-8 keys enter C storage. */
#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include <limits.h>
#include "bigtablemod.h"
#include "table.h"
#include <stdint.h>

typedef struct
{
    WORD reserved; /* Public ADT prefix; never trusted or interpreted as a native pointer. */
    Fht *table;    /* [GC:] Private scalar-only C owner, released by the matching heap finalizer. */
} TableOwner;

typedef struct
{
    WORD reserved; /* Public ADT prefix, deliberately devoid of authority. */
    FhtKeys *keys; /* [GC:] Independent C key snapshot, no references into either VM heap. */
} KeysOwner;

static Type *table_type;           /* [GC:] Runtime-lifetime exact descriptor for native table owners. */
static Type *keys_type;            /* [GC:] Runtime-lifetime exact descriptor for immutable snapshot owners. */
static unsigned long table_owners; /* [GC:] Live native scalar table owners, for explicit runtime statistics. */
static unsigned long key_owners;   /* [GC:] Live independent key snapshots, not references to table owners. */

/* Release private C storage during RC destruction or cycle sweep. No VM pointers are contained. */
static void free_table(Heap *heap, int swept)
{
    TableOwner *owner = H2D(TableOwner *, heap);
    USED(swept);
    if (owner->table)
    {
        table_owners--;
        fht_destroy(owner->table);
    }
    owner->table = nil;
}

/* Release snapshot storage exactly once when Dis retires its native owner. No user code runs. */
static void free_keys(Heap *heap, int swept)
{
    KeysOwner *owner = H2D(KeysOwner *, heap);
    USED(swept);
    if (owner->keys)
    {
        key_owners--;
        fht_keys_destroy(owner->keys);
    }
    owner->keys = nil;
}

/* Validate a borrowed table reference by exact private descriptor, rejecting forged public ADTs. */
static Fht *table_value(Bigtable_Handle *handle)
{
    if (handle == H || handle == nil || D2H(handle)->t != table_type || !((TableOwner *)handle)->table)
    {
        error("invalid native big table owner");
    }
    return ((TableOwner *)handle)->table;
}

/* Validate a borrowed snapshot reference before reading private metadata. */
static FhtKeys *keys_value(Bigtable_Keys *handle)
{
    if (handle == H || handle == nil || D2H(handle)->t != keys_type || !((KeysOwner *)handle)->keys)
    {
        error("invalid native big table keys owner");
    }
    return ((KeysOwner *)handle)->keys;
}

/* Build a numeric borrowed key; the C core rejects negatives before touching table storage. */
static FhtKey numeric_key(LONG number)
{
    FhtKey key = {FHT_INDEX, (int64_t)number, NULL, 0};
    return key;
}

/* Borrow the runtime UTF-8 cache and its explicit byte length, including embedded NUL.
 * Callers use/copy it before another conversion; no native table retains a Dis String pointer. */
static FhtKey string_key(String *text)
{
    FhtKey key = {FHT_TEXT, 0, NULL, 0};
    if (text != H && text != nil)
    {
        int i;
        key.text = string2c(text);
        if (text->len >= 0)
        {
            key.size = text->len;
        }
        else
        {
            for (i = 0; i < -text->len; i++)
            {
                char encoded[UTFmax];
                key.size += runetochar(encoded, &text->Srune[i]);
            }
        }
    }
    return key;
}

/* Publish a new rooted native table owner before allocating any C storage; frame owns the result.
 * [GC:] An allocation exception leaves either no owner or an owner with a null private pointer. */
void Bigtable_create(void *frame)
{
    F_Bigtable_create *call = frame;
    TableOwner *owner = H2D(TableOwner *, heapz(table_type));
    destroy(*call->ret);
    *call->ret = (Bigtable_Handle *)owner;
    owner->table = fht_create(NULL);
    if (!owner->table)
    {
        error(exNomem);
    }
    table_owners++;
}

/* Read a numeric key from the borrowed owner; failed reads return status plus an unusable zero. */
void Bigtable_getindex(void *frame)
{
    F_Bigtable_getindex *call = frame;
    int64_t value = 0;
    call->ret->t0 = fht_get(table_value(call->tab), numeric_key(call->key), &value);
    call->ret->t1 = value;
}

/* Read a copied string key; no method/metatable dispatch or application callback is performed. */
void Bigtable_gettext(void *frame)
{
    F_Bigtable_gettext *call = frame;
    int64_t value = 0;
    call->ret->t0 = fht_get(table_value(call->tab), string_key(call->key), &value);
    call->ret->t1 = value;
}

/* Insert a numeric big value at the sole scheduler owner; status reports allocation failure. */
void Bigtable_setindex(void *frame)
{
    F_Bigtable_setindex *call = frame;
    *call->ret = fht_set(table_value(call->tab), numeric_key(call->key), call->value);
}

/* Insert/copy a string key and scalar value without retaining a String heap pointer. */
void Bigtable_settext(void *frame)
{
    F_Bigtable_settext *call = frame;
    *call->ret = fht_set(table_value(call->tab), string_key(call->key), call->value);
}

/* Delete a numeric key without allocating or moving any other logical key. */
void Bigtable_delindex(void *frame)
{
    F_Bigtable_delindex *call = frame;
    *call->ret = fht_delete(table_value(call->tab), numeric_key(call->key));
}

/* Delete one string key and retire its copied native bytes. */
void Bigtable_deltext(void *frame)
{
    F_Bigtable_deltext *call = frame;
    *call->ret = fht_delete(table_value(call->tab), string_key(call->key));
}

/* Return sequence length in the generated exact big return slot. */
void Bigtable_length(void *frame)
{
    F_Bigtable_length *call = frame;
    *call->ret = (LONG)fht_length(table_value(call->tab));
}

/* Return all stored pairs, including dictionary keys and sparse numeric keys. */
void Bigtable_count(void *frame)
{
    F_Bigtable_count *call = frame;
    *call->ret = (LONG)fht_count(table_value(call->tab));
}

/* Root an empty destination owner first, then build a checked independent native slice.
 * Bounds or OOM raise explicitly. The source and old result remain GC-managed throughout. */
void Bigtable_slice(void *frame)
{
    F_Bigtable_slice *call = frame;
    Fht *source = table_value(call->tab);
    TableOwner *owner;
    FhtStatus status;
    if (call->first < 0 || call->end < 0)
    {
        error("invalid big table slice");
    }
    owner = H2D(TableOwner *, heapz(table_type));
    destroy(*call->ret);
    *call->ret = (Bigtable_Handle *)owner;
    status = fht_slice(source, call->first, call->end, &owner->table);
    if (status != FHT_OK)
    {
        error(status == FHT_NOMEM ? exNomem : "invalid big table slice");
    }
    table_owners++;
}

/* Root an independent immutable snapshot owner before copying native keys; no source alias escapes. */
void Bigtable_keysof(void *frame)
{
    F_Bigtable_keysof *call = frame;
    Fht *source = table_value(call->tab);
    KeysOwner *owner = H2D(KeysOwner *, heapz(keys_type));
    destroy(*call->ret);
    *call->ret = (Bigtable_Keys *)owner;
    if (fht_keys(source, &owner->keys) != FHT_OK)
    {
        error(exNomem);
    }
    key_owners++;
}

/* Return the complete key count of a validated immutable owner. */
void Bigtable_keylen(void *frame)
{
    F_Bigtable_keylen *call = frame;
    *call->ret = (LONG)keys_value(call->keys)->count;
}

/* Borrow a checked key until the native call returns. The caller frame roots its
 * immutable snapshot; no VM pointer is stored in C table/key storage. */
static FhtKey snapshot_key(Bigtable_Keys *owner, LONG index)
{
    FhtKeys *keys = keys_value(owner);
    if (index < 0 || (uint64_t)index >= keys->count)
    {
        error("big table key index out of range");
    }
    return keys->items[index];
}

/* Validate eagerly even if a key value is unused. Borrowed frame/owner; no allocation. */
void Bigtable_keycheck(void *frame)
{
    F_Bigtable_keycheck *call = frame;
    (void)snapshot_key(call->keys, call->index);
}

/* Read using an immutable key capability. Both frame-owned native owners remain
 * rooted for this call; status/value follow the existing checked get contract. */
void Bigtable_getkey(void *frame)
{
    F_Bigtable_getkey *call = frame;
    int64_t value = 0;
    FhtKey key = snapshot_key(call->keys, call->index);
    call->ret->t0 = fht_get(table_value(call->tab), key, &value);
    call->ret->t1 = value;
}

/* Write a scalar through a copied key capability; fht_set copies any new text bytes.
 * Borrowed owners never cross heaps, and allocation failure preserves logical contents. */
void Bigtable_setkey(void *frame)
{
    F_Bigtable_setkey *call = frame;
    FhtKey key = snapshot_key(call->keys, call->index);
    *call->ret = fht_set(table_value(call->tab), key, call->value);
}

/* Copy one snapshot key into a tagged tuple, preserving string bytes and numeric identity.
 * Existing result String is replaced only after successful conversion; invalid indices raise. */
void Bigtable_keyat(void *frame)
{
    F_Bigtable_keyat *call = frame;
    FhtKeys *keys = keys_value(call->keys);
    FhtKey key;
    String *text = H;
    if (call->index < 0 || (uint64_t)call->index >= keys->count)
    {
        error("big table key index out of range");
    }
    key = keys->items[call->index];
    if (key.kind == FHT_TEXT)
    {
        if (key.size > INT_MAX)
        {
            error("big table key too long");
        }
        text = c2string((char *)key.text, (int)key.size);
    }
    destroy(call->ret->t2);
    call->ret->t0 = key.kind;
    call->ret->t1 = key.kind == FHT_INDEX ? key.index : 0;
    call->ret->t2 = text;
}

/* Check a borrowed status word after a generated get/set call; no allocation or user code runs.
 * Missing, invalid and allocation failure raise explicitly; success emits no diagnostic traffic. */
void Bigtable_requireok(void *frame)
{
    F_Bigtable_requireok *call = frame;
    if (call->status != FHT_OK)
    {
        error(call->status == FHT_MISSING ? "missing key"
              : call->status == FHT_NOMEM ? "table allocation failure"
                                          : "invalid key/range");
    }
}

/* Validate a borrowed numeric key before capturing a FIR location; return normally for nonnegative keys. */
void Bigtable_indexcheck(void *frame)
{
    F_Bigtable_indexcheck *call = frame;
    if (call->key < 0)
    {
        error("invalid key/range");
    }
}

/* Validate copied half-open bounds before capturing a FIR range; source length is checked by slice. */
void Bigtable_rangecheck(void *frame)
{
    F_Bigtable_rangecheck *call = frame;
    if (call->first < 0 || call->end < call->first)
    {
        error("invalid key/range");
    }
}

/* Copy owner counts on the sole scheduler thread. Output pointers are borrowed/non-null;
 * no native addresses, keys or application values are exposed, and no allocation occurs. */
void dvm_bigtable_owners(unsigned long *tables, unsigned long *keys)
{
    *tables = table_owners;
    *keys = key_owners;
}

/* Register private scalar-only finalizers and generated typed exports after VM heap initialization.
 * Called once per experimental runtime process; this does not modify the owning Inferno sources. */
void dvm_bigtable_initialize(void)
{
    table_type = dtype(free_table, sizeof(TableOwner), nil, 0);
    keys_type = dtype(free_keys, sizeof(KeysOwner), nil, 0);
    if (!table_type || !keys_type)
    {
        error(exNomem);
    }
    builtinmod("$Bigtable", Bigtablemodtab, Bigtablemodlen);
}
