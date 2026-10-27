#include "table_owner.h"
#include <errno.h>


/* Create table before publishing owner; unwind either initialization failure. */
int fvm_table_owner_init(FvmTableOwner *owner, const FhtAllocator *allocator)
{
    int error;
    if (!owner)
    {
        return EINVAL;
    }
    owner->payload = fht_create(allocator);
    if (!owner->payload)
    {
        return ENOMEM;
    }
    error = fvm_owned_init(&owner->guard);
    if (error)
    {
        fht_destroy(owner->payload);
        owner->payload = NULL;
    }
    return error;
}


/* Guard a complete application transaction, including nested table helpers.
 * Payload and context are borrowed; normal callback returns always release. */
int fvm_table_owner_apply(
    FvmTableOwner *owner, uint64_t token, int (*callback)(Fht *, void *), void *context, int *callback_result)
{
    int error;
    if (!owner || !callback || !callback_result)
    {
        return EINVAL;
    }
    error = fvm_owned_enter(&owner->guard, token);
    if (error)
    {
        return error;
    }
    if (!owner->payload)
    {
        fvm_owned_leave(&owner->guard, token);
        return EINVAL;
    }
    *callback_result = callback(owner->payload, context);
    return fvm_owned_leave(&owner->guard, token);
}


/* Require external lifetime exclusion, then retire guard and table once. */
int fvm_table_owner_destroy(FvmTableOwner *owner)
{
    int error;
    if (!owner || !owner->payload)
    {
        return EINVAL;
    }
    error = fvm_owned_destroy(&owner->guard);
    if (error)
    {
        return error;
    }
    fht_destroy(owner->payload);
    owner->payload = NULL;
    return 0;
}
