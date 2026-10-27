/* Native scalar table owner; does not carry pointers between VM heaps. */
#ifndef FVM_TABLE_OWNER_H
#define FVM_TABLE_OWNER_H
#include "owned.h"
#include "table.h"

typedef struct
{
    FvmOwned guard; /* One operation guard shared by all references to this owner. */
    Fht *payload;   /* Owned scalar table, accessed only within apply callbacks. */
} FvmTableOwner;


/* Initialize unpublished borrowed owner with copied optional allocator.
 * Returns zero, ENOMEM, or native guard error. No cancellation or GC callbacks. */
int fvm_table_owner_init(FvmTableOwner *owner, const FhtAllocator *allocator);



/* Execute callback(payload, context) while operation token holds the guard.
 * Returns native error; callback_result is written only after callback completes.
 * Callback returns its own application status, must not longjmp/throw, and must
 * not retain payload pointers. Recursive callbacks must pass the same token.
 * Cancellation must be disabled throughout; all readers also use this boundary. */
int fvm_table_owner_apply(
    FvmTableOwner *owner, uint64_t token, int (*callback)(Fht *, void *), void *context, int *callback_result);



/* Destroy externally quiescent unpublished owner; rejects an active guard.
 * Returns native error or zero. No new users may race this lifetime boundary. */
int fvm_table_owner_destroy(FvmTableOwner *owner);
#endif
