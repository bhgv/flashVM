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
#include "table_leases.h"
#include <stddef.h>
#include <errno.h>
#include "relative.h"
#include <stdint.h>

typedef struct TableOwner TableOwner; /* Private nonmoving GC payload owner. */
typedef struct TableActor TableActor; /* One live process's trusted lease context. */
typedef struct TableWait TableWait;   /* One parked native call, never a wire record. */


/* Private table payload and scheduler-owned retained lease; no wire ownership. */
struct TableOwner
{
    WORD reserved;          /* Public ADT prefix; never native authority. */
    FvmTableOwner owned;    /* [GC:] Scalar storage and operation guard. */
    int ready;              /* Exact once-only initialization/finalization flag. */
    Prog *lease_owner;      /* Borrowed live scheduler process while pinned. */
    unsigned lease_depth;   /* Nested retained entries, excluding primitive calls. */
    TableOwner *lease_next; /* Borrowed next owner in its actor's held list. */
    TableWait *wait_head;   /* Owned cooperative wait nodes in FIFO order. */
    TableWait *wait_tail;   /* Borrowed final wait node. */
    unsigned wait_count;    /* VM waiters, separate from native condition waiters. */
};


/* Sole-executor process context owning leases and at most one blocked request. */
struct TableActor
{
    Prog *process;      /* Borrowed scheduler root; retired before process free. */
    uint64_t token;     /* Trusted local capability shared by one continuation. */
    TableOwner *held;   /* [GC:] Owners pinned through native root registration. */
    TableWait *waiting; /* Borrowed single parked call, if any. */
    TableActor *next;   /* Owned registry link on the sole VM owner thread. */
};


/* FIFO wait node owned by its pinned destination until wake or process abort. */
struct TableWait
{
    TableActor *actor; /* Borrowed live actor until wake/abort. */
    TableOwner *owner; /* Borrowed pinned owner, also rooted by the pending frame. */
    TableWait *next;   /* Owned FIFO link. */
};

#define DVM_TABLE_WAIT (Pbroken + 1) /* Host-private blocked state; no upstream enum change. */
static TableActor *table_actors;     /* Sole-executor contexts, not a cross-thread heap registry. */


/* Find/create a context on the bound owner; definition follows checked handle access. */
static TableActor *table_actor(Prog *process, int create);

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
    FvmTableOwner *source = request->operation == FVM_TABLE_SPLICE ? request->replacement : owner;
    TableActor *actor = table_actor(currun(), 0);
    uint64_t token = actor && actor->held ? actor->token : 0;
    int previous;
    int entered_owner = 0;
    int entered_source = 0;
    int failure = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous);
    if (failure)
    {
        error("native table cancellation state failure");
    }
    if (!token)
    {
        failure = fvm_owned_token(&token);
    }
    if (!failure)
    {
        failure = fvm_owned_try_enter(&owner->guard, token);
        entered_owner = !failure;
    }
    if (!failure && source != owner)
    {
        failure = fvm_owned_try_enter(&source->guard, token);
        entered_source = !failure;
    }
    if (!failure)
    {
        failure = fvm_table_request(owner, token, request);
    }
    if (entered_source && fvm_owned_leave(&source->guard, token))
    {
        failure = EINVAL;
    }
    if (entered_owner && fvm_owned_leave(&owner->guard, token))
    {
        failure = EINVAL;
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
    call->ret->t2 = (WORD)waiters + ((TableOwner *)call->tab)->wait_count;
}

/* Classify only mutable-table native calls; diagnostics and immutable conversions
 * never enter this gate. Return 1 or 2 table arguments, zero for unrelated calls. */
static unsigned table_call_arguments(void (*function)(void *))
{
    if (function == Bigtable_splice)
    {
        return 2;
    }
    if (function == Bigtable_getindex || function == Bigtable_gettext || function == Bigtable_setindex ||
        function == Bigtable_settext || function == Bigtable_delindex || function == Bigtable_deltext ||
        function == Bigtable_getkey || function == Bigtable_setkey || function == Bigtable_delkey ||
        function == Bigtable_length || function == Bigtable_count || function == Bigtable_slice ||
        function == Bigtable_slicerange || function == Bigtable_keysof || function == Bigtable_resolveindex ||
        function == Bigtable_acquire)
    {
        return 1;
    }
    return 0;
}

/* Find a process's private lease context; create only when requested.
 * Contexts borrow rooted scheduler processes and never migrate executor threads. */
static TableActor *table_actor(Prog *process, int create)
{
    TableActor *actor;
    for (actor = table_actors; actor; actor = actor->next)
    {
        if (actor->process == process)
        {
            return actor;
        }
    }
    if (!create)
    {
        return nil;
    }
    actor = calloc(1, sizeof(*actor));
    if (!actor)
    {
        error(exNomem);
    }
    actor->process = process;
    actor->next = table_actors;
    table_actors = actor;
    return actor;
}

/* Retire an empty context only; never disturb a live owner or parked request. */
static void table_actor_idle(TableActor *actor)
{
    TableActor **slot;
    if (!actor || actor->held || actor->waiting)
    {
        return;
    }
    for (slot = &table_actors; *slot && *slot != actor; slot = &(*slot)->next)
    {
    }
    if (*slot)
    {
        *slot = actor->next;
        free(actor);
    }
}

/* Validate the complete pending builtin frame and its required argument roots.
 * A parked uncalled frame remains in SP's typed stack walk until retry/cleanup. */
static void table_call_frame(void (*function)(void *), void *frame)
{
    Frame header;
    Type *type;
    Runtab *entry;
    int i;
    memmove(&header, frame, sizeof(header));
    type = header.t ? header.t : SEXTYPE((Frame *)frame)->reg.TR;
    if (type == nil || type == H)
    {
        error(exType);
    }
    for (entry = Bigtablemodtab; entry->name; entry++)
    {
        if (entry->fn != function)
        {
            continue;
        }
        if (type->size < entry->size || type->np < entry->np)
        {
            error(exType);
        }
        for (i = 0; i < entry->np; i++)
        {
            if ((type->map[i] & entry->map[i]) != entry->map[i])
            {
                error(exType);
            }
        }
        return;
    }
    error(exType);
}

/* Publish one cooperative waiter without holding a native metadata mutex.
 * No program callback/GC runs after delrun until the prepared instruction returns. */
static void table_park(TableOwner *owner, TableActor *actor)
{
    TableWait *waiter;
    if (actor->waiting || actor->process != currun())
    {
        error("invalid table lease wait state");
    }
    waiter = calloc(1, sizeof(*waiter));
    if (!waiter)
    {
        error(exNomem);
    }
    waiter->actor = actor;
    waiter->owner = owner;
    if (owner->wait_tail)
    {
        owner->wait_tail->next = waiter;
    }
    else
    {
        owner->wait_head = waiter;
    }
    owner->wait_tail = waiter;
    owner->wait_count++;
    actor->waiting = waiter;
    /* The retained host uses bytecode PCs even when guarded leaves run in JIT. */
    DVM_R.PC--;
    DVM_R.IC = 1; /* End this instruction quantum before another ready actor runs. */
    DVM_R.t = 1;  /* Match retained scheduler-blocking instruction discipline. */
    delrun(DVM_TABLE_WAIT);
}

/* Wake FIFO contenders only after final payload release. They retry the same
 * native call with unchanged rooted argument frames; no stale result is written. */
static void table_wake(TableOwner *owner)
{
    TableWait *waiter;
    while ((waiter = owner->wait_head) != nil)
    {
        TableActor *actor = waiter->actor;
        owner->wait_head = waiter->next;
        actor->waiting = nil;
        if (actor->process->state == DVM_TABLE_WAIT)
        {
            addrun(actor->process);
        }
        free(waiter);
        table_actor_idle(actor);
    }
    owner->wait_tail = nil;
    owner->wait_count = 0;
}

/* Admit or cooperatively park a mutable-table call before MCALL changes FP/M.
 * Stable ordering rejects a wait below an already-held owner, avoiding cycles.
 * Idle scalar calls can still read an unleased lower owner without suspension. */
int dvm_bigtable_call_ready(void (*function)(void *), void *frame)
{
    unsigned count = table_call_arguments(function);
    Bigtable_Handle *handle;
    TableOwner *owners[2];
    TableActor *actor;
    Prog *process;
    uintptr highest = 0;
    unsigned i;
    if (!count)
    {
        return 1;
    }
    table_call_frame(function, frame);
    memmove(&handle, (uchar *)frame + offsetof(F_Bigtable_length, tab), sizeof(handle));
    owners[0] = (TableOwner *)handle;
    (void)table_owner(handle);
    if (count == 2)
    {
        memmove(&handle, (uchar *)frame + offsetof(F_Bigtable_splice, replacement), sizeof(handle));
        owners[1] = (TableOwner *)handle;
        (void)table_owner(handle);
    }
    process = currun();
    actor = table_actor(process, 0);
    if (actor)
    {
        TableOwner *held;
        for (held = actor->held; held; held = held->lease_next)
        {
            if ((uintptr)&held->owned.guard > highest)
            {
                highest = (uintptr)&held->owned.guard;
            }
        }
    }
    if (function == Bigtable_acquire && owners[0]->lease_owner != process && highest &&
        (uintptr)&owners[0]->owned.guard < highest)
    {
        error("table lease order violation");
    }
    for (i = 0; i < count; i++)
    {
        if (owners[i]->lease_owner && owners[i]->lease_owner != process)
        {
            if (highest && (uintptr)&owners[i]->owned.guard < highest)
            {
                error("table lease order violation");
            }
            table_park(owners[i], actor ? actor : table_actor(process, 1));
            return 0;
        }
    }
    return 1;
}

/* Release all recursive levels of one pinned owner. Every caller removes it
 * from its actor's list first. Wake requests before dropping the extra root. */
static void table_lease_drop(TableOwner *owner, TableActor *actor)
{
    while (owner->lease_depth)
    {
        if (fvm_owned_leave(&owner->owned.guard, actor->token))
        {
            sysfatal("unbalanced native table lease");
        }
        owner->lease_depth--;
    }
    owner->lease_owner = nil;
    owner->lease_next = nil;
    table_wake(owner);
    ptrdel(D2H(owner));
    destroy(owner);
}

/* Retire a parked request and all pinned leases before unwinding application
 * frames. Does not allocate, enter a payload lock or throw through a live lease. */
void dvm_bigtable_abort(Prog *process)
{
    TableActor *actor = table_actor(process, 0);
    if (!actor)
    {
        return;
    }
    if (actor->waiting)
    {
        TableWait *waiter = actor->waiting;
        TableOwner *owner = waiter->owner;
        TableWait **slot;
        TableWait *last = nil;
        for (slot = &owner->wait_head; *slot && *slot != waiter; slot = &(*slot)->next)
        {
            last = *slot;
        }
        if (*slot)
        {
            *slot = waiter->next;
            if (owner->wait_tail == waiter)
            {
                owner->wait_tail = last;
            }
            owner->wait_count--;
        }
        actor->waiting = nil;
        free(waiter);
    }
    while (actor->held)
    {
        TableOwner *owner = actor->held;
        actor->held = owner->lease_next;
        table_lease_drop(owner, actor);
    }
    table_actor_idle(actor);
}

/* Pin one table across source instructions/channel waits on its owning process.
 * The executor never blocks on another actor: preflight queues before this call.
 * Nested acquisition shares token/owner and adds exactly one recursion level. */
void Bigtable_acquire(void *frame)
{
    F_Bigtable_acquire *call = frame;
    TableOwner *owner = (TableOwner *)call->tab;
    TableActor *actor = table_actor(currun(), 1);
    int failure;
    (void)table_owner(call->tab);
    if (!actor->token && fvm_owned_token(&actor->token))
    {
        error("table lease token failure");
    }
    if (owner->lease_owner && owner->lease_owner != currun())
    {
        error("table lease preflight missing");
    }
    if (!owner->lease_depth)
    {
        Heap *heap = D2H(owner);
        if (heap->ref == 0 || heap->ref == (ulong)-1)
        {
            error(exHeap);
        }
        ptradd(heap);
        heap->ref++;
        Setmark(heap);
    }
    failure = fvm_owned_try_enter(&owner->owned.guard, actor->token);
    if (failure)
    {
        if (!owner->lease_depth)
        {
            ptrdel(D2H(owner));
            destroy(owner);
        }
        error("table lease acquisition failure");
    }
    if (!owner->lease_depth)
    {
        owner->lease_next = actor->held;
        actor->held = owner;
    }
    owner->lease_owner = currun();
    owner->lease_depth++;
}

/* Leave one application lease level; a foreign process cannot release an owner.
 * Final release wakes waiters and retires the private pin, not caller references. */
void Bigtable_release(void *frame)
{
    F_Bigtable_release *call = frame;
    TableOwner *owner = (TableOwner *)call->tab;
    TableActor *actor = table_actor(currun(), 0);
    TableOwner **slot;
    (void)table_owner(call->tab);
    if (!actor || owner->lease_owner != currun() || !owner->lease_depth)
    {
        error("table lease not owned");
    }
    if (owner->lease_depth > 1)
    {
        if (fvm_owned_leave(&owner->owned.guard, actor->token))
        {
            error("table lease release failure");
        }
        owner->lease_depth--;
        return;
    }
    for (slot = &actor->held; *slot && *slot != owner; slot = &(*slot)->lease_next)
    {
    }
    if (!*slot)
    {
        error("table lease owner missing");
    }
    *slot = owner->lease_next;
    table_lease_drop(owner, actor);
    table_actor_idle(actor);
}
