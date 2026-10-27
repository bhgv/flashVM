/* Native big-table boundary. Only exact scalar payloads and copied UTF-8 keys enter C storage. */
#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include <limits.h>
#include "bigtablemod.h"
#include "table.h"
#include "table_request.h"
#include <errno.h>
#include "relative.h"
#include <stdint.h>

typedef struct
{
    WORD reserved;       /* Public ADT prefix; never trusted or interpreted as a native pointer. */
    FvmTableOwner owned; /* [GC:] Scalar payload plus operation guard; private, stable nonmoving owner. */
    int ready;           /* Guard and payload initialized; finalization retires them exactly once. */
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
    if (owner->ready)
    {
        if (fvm_table_owner_destroy(&owner->owned))
        {
            sysfatal("finalizing an active native table owner");
        }
        table_owners--;
        owner->ready = 0;
    }
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
static FvmTableOwner *table_owner(Bigtable_Handle *handle)
{
    TableOwner *owner = (TableOwner *)handle;
    if (handle == H || handle == nil || D2H(handle)->t != table_type || !owner->ready)
    {
        error("invalid native big table owner");
    }
    return &owner->owned;
}


/* Execute a complete scalar request on the VM owner thread. Native cancellation
 * stays disabled while guards are live; runtime errors occur only after release.
 * Requests borrow frame-rooted owners/keys and return scalar/native snapshots. */
static void table_apply(Bigtable_Handle *handle, FvmTableRequest *request)
{
    FvmTableOwner *owner = table_owner(handle);
    uint64_t token;
    int previous;
    int failure = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous);
    if (failure)
    {
        error("native table cancellation state failure");
    }
    failure = fvm_owned_token(&token);
    if (!failure)
    {
        failure = fvm_table_request(owner, token, request);
    }
    pthread_setcancelstate(previous, NULL);
    if (failure)
    {
        error("native table guard failure");
    }
}


/* Initialize an unpublished private owner; finalization can retire partial
 * construction after a VM exception. No heap reference escapes into C storage. */
static void initialize_table(TableOwner *owner)
{
    int failure = fvm_table_owner_init(&owner->owned, NULL);
    if (failure)
    {
        error(failure == ENOMEM ? exNomem : "native table guard initialization failure");
    }
    owner->ready = 1;
    table_owners++;
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


/* Build a numeric borrowed key; relative positions resolve under its payload guard. */
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
    initialize_table(owner);
}


/* Read a numeric key from the borrowed owner; failed reads return status plus an unusable zero. */
void Bigtable_getindex(void *frame)
{
    F_Bigtable_getindex *call = frame;
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_GET;
    request.key = numeric_key(call->key);
    table_apply(call->tab, &request);
    call->ret->t0 = request.status;
    call->ret->t1 = request.status == FHT_OK ? request.value : 0;
}


/* Read a copied string key; no method/metatable dispatch or application callback is performed. */
void Bigtable_gettext(void *frame)
{
    F_Bigtable_gettext *call = frame;
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_GET;
    request.key = string_key(call->key);
    table_apply(call->tab, &request);
    call->ret->t0 = request.status;
    call->ret->t1 = request.status == FHT_OK ? request.value : 0;
}


/* Insert a numeric big value at the sole scheduler owner; status reports allocation failure. */
void Bigtable_setindex(void *frame)
{
    F_Bigtable_setindex *call = frame;
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_SET;
    request.key = numeric_key(call->key);
    request.value = call->value;
    table_apply(call->tab, &request);
    *call->ret = request.status;
}


/* Insert/copy a string key and scalar value without retaining a String heap pointer. */
void Bigtable_settext(void *frame)
{
    F_Bigtable_settext *call = frame;
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_SET;
    request.key = string_key(call->key);
    request.value = call->value;
    table_apply(call->tab, &request);
    *call->ret = request.status;
}


/* Delete a numeric key without allocating or moving any other logical key. */
void Bigtable_delindex(void *frame)
{
    F_Bigtable_delindex *call = frame;
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_DELETE;
    request.key = numeric_key(call->key);
    table_apply(call->tab, &request);
    *call->ret = request.status;
}


/* Delete one string key and retire its copied native bytes. */
void Bigtable_deltext(void *frame)
{
    F_Bigtable_deltext *call = frame;
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_DELETE;
    request.key = string_key(call->key);
    table_apply(call->tab, &request);
    *call->ret = request.status;
}


/* Return sequence length in the generated exact big return slot. */
void Bigtable_length(void *frame)
{
    F_Bigtable_length *call = frame;
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_LENGTH;
    table_apply(call->tab, &request);
    *call->ret = request.value;
}


/* Return all stored pairs, including dictionary keys and sparse numeric keys. */
void Bigtable_count(void *frame)
{
    F_Bigtable_count *call = frame;
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_COUNT;
    table_apply(call->tab, &request);
    *call->ret = request.value;
}


/* Root an empty destination owner first, then build a checked independent native slice.
 * Bounds or OOM raise explicitly. The source and old result remain GC-managed throughout. */
void Bigtable_slice(void *frame)
{
    F_Bigtable_slice *call = frame;
    FvmTableRequest request = {0};
    TableOwner *owner = H2D(TableOwner *, heapz(table_type));
    destroy(*call->ret);
    *call->ret = (Bigtable_Handle *)owner;
    initialize_table(owner);
    request.operation = FVM_TABLE_SLICE;
    request.first = call->first;
    request.end = call->end;
    table_apply(call->tab, &request);
    if (request.status != FHT_OK)
    {
        error(request.status == FHT_NOMEM ? exNomem : "invalid big table slice");
    }
    /* This destination is private to the current builtin; no other task can
     * observe it before return. Adopt the independently allocated source copy. */
    fht_destroy(owner->owned.payload);
    owner->owned.payload = request.table;
}


/* Resolve signed relative bounds against a validated live owner before RHS.
 * Return only absolute scalar positions; invalid ranges raise without mutation. */
void Bigtable_slicerange(void *frame)
{
    F_Bigtable_slicerange *call = frame;
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_RANGE;
    request.first = call->first;
    request.end = call->end;
    table_apply(call->tab, &request);
    if (request.status != FHT_OK)
    {
        error("invalid key/range");
    }
    call->ret->t0 = request.first;
    call->ret->t1 = request.end;
}


/* Replace a captured absolute range using the native rollback-safe table core.
 * Both owners remain rooted by the builtin frame. This host executes one C
 * instruction at a time; external cross-thread payload access is not admitted. */
void Bigtable_splice(void *frame)
{
    F_Bigtable_splice *call = frame;
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_SPLICE;
    request.first = call->first;
    request.end = call->end;
    request.replacement = table_owner(call->replacement);
    table_apply(call->tab, &request);
    *call->ret = request.status;
}


/* Return scalar count for a borrowed exact String owner, including nil-as-empty H.
 * Reject invalid descriptors/representations before a rune read or allocation. */
static unsigned splice_string_length(String *text)
{
    unsigned length;
    if (text == H)
    {
        return 0;
    }
    if (text == nil || D2H(text)->t != &Tstring || text->len == INT_MIN)
    {
        error(exHeap);
    }
    length = text->len < 0 ? (unsigned)-text->len : (unsigned)text->len;
    if (text->max < 0 || length > (unsigned)text->max)
    {
        error(exHeap);
    }
    return length;
}


/* Copy a checked rune span into independently allocated destination storage.
 * Count/offsets are rune positions. Widen ASCII only when needed; otherwise
 * use a contiguous copy. No allocation, callbacks or scheduler switch occurs. */
static void splice_string_copy(String *destination, unsigned offset, String *source, unsigned first, unsigned count)
{
    unsigned i;
    if (!count)
    {
        return;
    }
    if (destination->len < 0)
    {
        if (source->len < 0)
        {
            memmove(destination->Srune + offset, source->Srune + first, (size_t)count * sizeof(Rune));
        }
        else
        {
            for (i = 0; i < count; i++)
            {
                destination->Srune[offset + i] = (uchar)source->Sascii[first + i];
            }
        }
    }
    else
    {
        /* Caller selects narrow output only when both inputs are narrow. */
        memmove(destination->Sascii + offset, source->Sascii + first, count);
    }
}


/* Capture normalized absolute exclusive rune positions before source RHS.
 * The builtin frame roots the immutable source until the caller retains it. */
void Bigtable_stringrange(void *frame)
{
    F_Bigtable_stringrange *call = frame;
    uint64_t first;
    uint64_t end;
    if (!fvm_relative_range(splice_string_length(call->text), call->first, call->end, &first, &end))
    {
        error("invalid key/range");
    }
    call->ret->t0 = (LONG)first;
    call->ret->t1 = (LONG)end;
}


/* Build a complete independent rune string before replacing the rooted result.
 * All inputs are borrowed frame roots; self replacement/overlap is safe. Bounds
 * are captured absolute positions. Oversized results or OOM raise before commit. */
void Bigtable_stringsplice(void *frame)
{
    F_Bigtable_stringsplice *call = frame;
    unsigned length = splice_string_length(call->text);
    unsigned replacement = splice_string_length(call->replacement);
    uint64_t total;
    int wide;
    String *result;
    if (call->first < 0 || call->end < call->first || (uint64_t)call->end > length)
    {
        error("invalid key/range");
    }
    total = length - ((uint64_t)call->end - (uint64_t)call->first) + replacement;
    if (total > INT_MAX)
    {
        error(exNomem);
    }
    wide = (call->text != H && call->text->len < 0) || (call->replacement != H && call->replacement->len < 0);
    result = wide ? newrunes((int)total) : newstring((int)total);
    splice_string_copy(result, 0, call->text, 0, (unsigned)call->first);
    splice_string_copy(result, (unsigned)call->first, call->replacement, 0, replacement);
    splice_string_copy(
        result, (unsigned)call->first + replacement, call->text, (unsigned)call->end, length - (unsigned)call->end);
    destroy(*call->ret);
    *call->ret = result;
}


/* Capture one existing Unicode rune, including negative indexes from the end.
 * Return absolute exclusive bounds; invalid positions raise before RHS runs. */
void Bigtable_stringindex(void *frame)
{
    F_Bigtable_stringindex *call = frame;
    unsigned length = splice_string_length(call->text);
    uint64_t index;
    if (!fvm_relative_index(length, call->index, &index) || index >= length)
    {
        error("invalid key/range");
    }
    call->ret->t0 = (LONG)index;
    call->ret->t1 = (LONG)index + 1;
}


/* Allocate one valid Unicode scalar, including U+0000; reject surrogate values.
 * Result publication follows allocation and initialization; old aliases survive. */
void Bigtable_runetext(void *frame)
{
    F_Bigtable_runetext *call = frame;
    String *result;
    if (call->value < 0 || call->value > 0x10ffff || (call->value >= 0xd800 && call->value <= 0xdfff))
    {
        error("invalid key/range");
    }
    result = newrunes(1);
    result->Srune[0] = (Rune)call->value;
    destroy(*call->ret);
    *call->ret = result;
}


/* Root an independent immutable snapshot owner before copying native keys; no source alias escapes. */
void Bigtable_keysof(void *frame)
{
    F_Bigtable_keysof *call = frame;
    FvmTableRequest request = {0};
    KeysOwner *owner = H2D(KeysOwner *, heapz(keys_type));
    destroy(*call->ret);
    *call->ret = (Bigtable_Keys *)owner;
    request.operation = FVM_TABLE_KEYS;
    table_apply(call->tab, &request);
    if (request.status != FHT_OK)
    {
        error(exNomem);
    }
    owner->keys = request.keys;
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


/* Publish a fresh reflected name into a borrowed typed return slot. The old String
 * is released after successful allocation; no application callback or conversion runs. */
static void type_name(String **result, WORD kind)
{
    static const char *const names[] = {"big", "string", "table of big", "keys", "int"};
    String *text;
    if (kind < 0 || kind >= 5)
    {
        error("invalid big table type kind");
    }
    text = c2string((char *)names[kind], (int)strlen(names[kind]));
    destroy(*result);
    *result = text;
}


/* Parse a borrowed copied text key as strict signed decimal. No partial result
 * or locale conversion is exposed; the native frame retains its source owner. */
static LONG converted_big(FhtKey key)
{
    int64_t value = 0;
    if (key.kind == FHT_INDEX)
    {
        return key.index;
    }
    if (fht_big_parse(key.text, key.size, &value) != FHT_OK)
    {
        error("invalid big conversion");
    }
    return value;
}


/* Publish copied text for a borrowed key. Numeric formatting is exact, and text
 * keeps its byte length including embedded zeros; no native pointer escapes. */
static void converted_text(String **result, FhtKey key)
{
    char digits[21];
    String *text;
    if (key.kind == FHT_INDEX)
    {
        key.size = fht_big_format(key.index, digits, sizeof(digits));
        key.text = digits;
    }
    if (key.size > INT_MAX)
    {
        error("big table key too long");
    }
    text = c2string((char *)key.text, (int)key.size);
    destroy(*result);
    *result = text;
}


/* Convert borrowed String bytes to a copied big; invalid syntax/overflow raises. */
void Bigtable_bigfromtext(void *frame)
{
    F_Bigtable_bigfromtext *call = frame;
    *call->ret = converted_big(string_key(call->text));
}


/* Format a copied big exactly; the caller's return String is replaced on success. */
void Bigtable_textfrombig(void *frame)
{
    F_Bigtable_textfrombig *call = frame;
    converted_text(call->ret, numeric_key(call->value));
}


/* Convert the actual kind of a rooted snapshot key; bad owners/positions raise first. */
void Bigtable_keybig(void *frame)
{
    F_Bigtable_keybig *call = frame;
    *call->ret = converted_big(snapshot_key(call->keys, call->index));
}


/* Copy text from the actual snapshot key kind while its native caller frame roots it. */
void Bigtable_keytext(void *frame)
{
    F_Bigtable_keytext *call = frame;
    converted_text(call->ret, snapshot_key(call->keys, call->index));
}


/* Reflect a supported static source type; input is a stable module tag, not a VM address. */
void Bigtable_typename(void *frame)
{
    F_Bigtable_typename *call = frame;
    type_name(call->ret, call->kind);
}


/* Reflect the actual numeric/text kind of a retained snapshot key. The caller frame
 * roots its owner across String allocation; invalid owners/positions raise first. */
void Bigtable_keytype(void *frame)
{
    F_Bigtable_keytype *call = frame;
    FhtKey key = snapshot_key(call->keys, call->index);
    type_name(call->ret, key.kind == FHT_INDEX ? 0 : 1);
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
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_GET;
    request.key = snapshot_key(call->keys, call->index);
    table_apply(call->tab, &request);
    call->ret->t0 = request.status;
    call->ret->t1 = request.status == FHT_OK ? request.value : 0;
}


/* Write a scalar through a copied key capability; fht_set copies any new text bytes.
 * Borrowed owners never cross heaps, and allocation failure preserves logical contents. */
void Bigtable_setkey(void *frame)
{
    F_Bigtable_setkey *call = frame;
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_SET;
    request.key = snapshot_key(call->keys, call->index);
    request.value = call->value;
    table_apply(call->tab, &request);
    *call->ret = request.status;
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


/* Capture a canonical numeric key before RHS effects can change prefix length.
 * frame is borrowed; invalid relative bounds raise without result publication. */
void Bigtable_resolveindex(void *frame)
{
    F_Bigtable_resolveindex *call = frame;
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_RESOLVE;
    request.key = numeric_key(call->key);
    table_apply(call->tab, &request);
    if (request.status != FHT_OK)
    {
        error("invalid key/range");
    }
    *call->ret = request.key.index;
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
    if (call->first >= 0 && call->end >= 0 && call->end < call->first)
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


/* Convert a borrowed get/delete status to a normalized bool. Missing is ordinary
 * absence; invalid/OOM statuses raise before returning. No allocation or blocking.
 */
void Bigtable_present(void *frame)
{
    F_Bigtable_present *call = frame;
    if (call->status != FHT_OK && call->status != FHT_MISSING)
    {
        error(call->status == FHT_NOMEM ? "table allocation failure" : "invalid key/range");
    }
    *call->ret = call->status == FHT_OK;
}


/* Delete a snapshot-owned key from a borrowed table. Both owners are rooted by
 * the native call frame; no reference/address crosses heaps and no key is moved.
 * Invalid capabilities raise; absent entries return Missing without mutation.
 */
void Bigtable_delkey(void *frame)
{
    F_Bigtable_delkey *call = frame;
    FvmTableRequest request = {0};
    request.operation = FVM_TABLE_DELETE;
    request.key = snapshot_key(call->keys, call->index);
    table_apply(call->tab, &request);
    *call->ret = request.status;
}


/* Inspect only guard metadata, without waiting for payload ownership. The
 * caller frame roots this owner. State IDs are diagnostics, not remote authority. */
void Bigtable_guardstate(void *frame)
{
    F_Bigtable_guardstate *call = frame;
    FvmTableOwner *owner = table_owner(call->tab);
    uint64_t token;
    unsigned depth;
    unsigned waiters;
    if (fvm_owned_inspect(&owner->guard, &token, &depth, &waiters))
    {
        error("native table guard inspection failure");
    }
    memmove(&call->ret->t0, &token, sizeof(token));
    call->ret->t1 = depth;
    call->ret->t2 = waiters;
}
