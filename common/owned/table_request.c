#include "table_request.h"
#include "relative.h"
#include <errno.h>

static pthread_mutex_t token_mutex = PTHREAD_MUTEX_INITIALIZER; /* Short process-local token counter guard. */
static uint64_t token_serial; /* Last issued token; never wraps or reuses an operation capability. */


/* Allocate one capability without touching any payload or VM scheduler state. */
int fvm_owned_token(uint64_t *result)
{
    int error;
    if (!result)
    {
        return EINVAL;
    }
    error = pthread_mutex_lock(&token_mutex);
    if (error)
    {
        return error;
    }
    if (token_serial == UINT64_MAX)
    {
        error = EOVERFLOW;
    }
    else
    {
        *result = ++token_serial;
    }
    pthread_mutex_unlock(&token_mutex);
    return error;
}


/* Sort borrowed guard pointers using stable native integer addresses, not C's
 * undefined relational comparison of pointers into distinct allocated owners. */
static void ordered(FvmOwned **first, FvmOwned **second)
{
    if ((uintptr_t)*first > (uintptr_t)*second)
    {
        FvmOwned *swap = *first;
        *first = *second;
        *second = swap;
    }
}


/* Acquire the lower owner first, preserving existing recursive ownership.
 * Inspect only metadata when checking an inverted partially-held pair. */
int fvm_owned_enter_pair(FvmOwned *first, FvmOwned *second, uint64_t token)
{
    uint64_t low_owner;
    uint64_t high_owner;
    unsigned depth;
    unsigned waiters;
    int error;
    if (!first || !second || !token)
    {
        return EINVAL;
    }
    ordered(&first, &second);
    if (first == second)
    {
        return fvm_owned_enter(first, token);
    }
    error = fvm_owned_inspect(first, &low_owner, &depth, &waiters);
    if (!error)
    {
        error = fvm_owned_inspect(second, &high_owner, &depth, &waiters);
    }
    if (error)
    {
        return error;
    }
    if (high_owner == token && low_owner != token)
    {
        return EDEADLK;
    }
    error = fvm_owned_enter(first, token);
    if (error)
    {
        return error;
    }
    error = fvm_owned_enter(second, token);
    if (error)
    {
        fvm_owned_leave(first, token);
    }
    return error;
}


/* Prevalidate matching ownership before releasing either recursion level.
 * A token has one continuation, so its own observed ownership cannot race away. */
int fvm_owned_leave_pair(FvmOwned *first, FvmOwned *second, uint64_t token)
{
    uint64_t owner;
    unsigned depth;
    unsigned waiters;
    int error;
    if (!first || !second || !token)
    {
        return EINVAL;
    }
    ordered(&first, &second);
    error = fvm_owned_inspect(first, &owner, &depth, &waiters);
    if (error || owner != token)
    {
        return error ? error : EPERM;
    }
    if (first == second)
    {
        return fvm_owned_leave(first, token);
    }
    error = fvm_owned_inspect(second, &owner, &depth, &waiters);
    if (error || owner != token)
    {
        return error ? error : EPERM;
    }
    error = fvm_owned_leave(second, token);
    if (!error)
    {
        error = fvm_owned_leave(first, token);
    }
    return error;
}


/* EN: Insert one scalar into the dense prefix using transactional splice.
 * RU: Вставить скаляр в плотную часть через транзакционный splice. */
static FhtStatus insert_scalar(Fht *table, int64_t position, int64_t value)
{
    if (position < 0 || (uint64_t)position > fht_length(table))
    {
        return FHT_INVALID;
    }
    Fht *replacement = fht_create(NULL);
    if (!replacement)
    {
        return FHT_NOMEM;
    }
    FhtKey zero = {.kind = FHT_INDEX};
    FhtStatus status = fht_set(replacement, zero, value);
    if (status == FHT_OK)
    {
        status = fht_splice(table, position, position, replacement);
    }
    fht_destroy(replacement);
    return status;
}


/* EN: Remove the last dense cell while preserving all sparse and text keys.
 * RU: Удалить последнюю плотную ячейку, сохраняя разреженные и строковые ключи. */
static FhtStatus pop_scalar(Fht *table, int64_t *output)
{
    uint64_t length = fht_length(table);
    if (!length)
    {
        return FHT_MISSING;
    }
    FhtKey last = {.kind = FHT_INDEX, .index = (int64_t)(length - 1)};
    int64_t value;
    FhtStatus status = fht_get(table, last, &value);
    if (status == FHT_OK)
    {
        status = fht_delete(table, last);
    }
    if (status == FHT_OK)
    {
        *output = value;
    }
    return status;
}


/* Execute scalar-only storage operations after all required guards are held.
 * Borrowed request inputs stay live through return. New snapshots transfer to
 * the request only on success. No runtime allocation/exception callback is used. */
static FhtStatus execute(FvmTableOwner *owner, FvmTableRequest *request)
{
    uint64_t first;
    uint64_t end;
    switch (request->operation)
    {
    case FVM_TABLE_GET:
        return fht_get(owner->payload, request->key, &request->value);
    case FVM_TABLE_SET:
        return fht_set(owner->payload, request->key, request->value);
    case FVM_TABLE_DELETE:
        return fht_delete(owner->payload, request->key);
    case FVM_TABLE_LENGTH:
        request->value = (int64_t)fht_length(owner->payload);
        return FHT_OK;
    case FVM_TABLE_COUNT:
        request->value = (int64_t)fht_count(owner->payload);
        return FHT_OK;
    case FVM_TABLE_RESOLVE:
        return fht_resolve(owner->payload, request->key, &request->key);
    case FVM_TABLE_RANGE:
        if (!fvm_relative_range(fht_length(owner->payload), request->first, request->end, &first, &end) ||
            first > INT64_MAX || end > INT64_MAX)
        {
            return FHT_INVALID;
        }
        request->first = (int64_t)first;
        request->end = (int64_t)end;
        return FHT_OK;
    case FVM_TABLE_SLICE:
        return fht_slice_relative(owner->payload, request->first, request->end, &request->table);
    case FVM_TABLE_KEYS:
        return fht_keys(owner->payload, &request->keys);
    case FVM_TABLE_SPLICE:
        return fht_splice(owner->payload, request->first, request->end, request->replacement->payload);
    case FVM_TABLE_INSERT:
        return insert_scalar(owner->payload, request->first, request->value);
    case FVM_TABLE_PUSH:
        first = fht_length(owner->payload);
        return first > INT64_MAX ? FHT_INVALID : insert_scalar(owner->payload, (int64_t)first, request->value);
    case FVM_TABLE_POP:
        return pop_scalar(owner->payload, &request->value);
    case FVM_TABLE_ADD:
    {
        int64_t old;
        FhtStatus found = fht_get(owner->payload, request->key, &old);
        return found == FHT_MISSING ? fht_set(owner->payload, request->key, request->value) :
            found == FHT_OK ? FHT_INVALID : found;
    }
    default:
        return FHT_INVALID;
    }
}


/* Keep application errors as ordinary results, then balance guard ownership.
 * Caller owns initialized request/result lifetimes and must not throw through
 * this C boundary. Pair ordering also makes reversed cross-copies safe. */
int fvm_table_request(FvmTableOwner *owner, uint64_t token, FvmTableRequest *request)
{
    FvmTableOwner *source;
    int error;
    if (!owner || !request)
    {
        return EINVAL;
    }
    source = request->operation == FVM_TABLE_SPLICE ? request->replacement : owner;
    if (!source)
    {
        return EINVAL;
    }
    error = fvm_owned_enter_pair(&owner->guard, &source->guard, token);
    if (error)
    {
        return error;
    }
    request->status = owner->payload && source->payload ? execute(owner, request) : FHT_INVALID;
    return fvm_owned_leave_pair(&owner->guard, &source->guard, token);
}
