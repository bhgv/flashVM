/* Owner-local reference ledger. Native transfer requires external publication. */
#include "disvm/owner_scope.h"


/* Test scope affinity BEFORE reading owner-mutated head/busy. scope storage is
 * borrowed/stable and its context immutable until owner-only close. Foreign
 * calls require synchronized publication and no concurrent scope retirement. */
static int available(DVM_OwnerScope *scope)
{
    DVM_NativeContext *context = dvm_native_context_current();
    return scope && context && scope->context == context && !scope->busy &&
        !dvm_native_context_object_count(context);
}


/* Check privately owned entry storage. entry borrowed; no callback or mutation.
 * Caller must not race another owner editing this same entry. */
static int empty(DVM_OwnerEntry *entry)
{
    return entry && !entry->scope && !entry->next && !entry->previous &&
        !entry->object && !entry->ops;
}


/* Insert one already-owned object without callback/allocation. scope and entry
 * are validated private owner storage; ops/object remain live through insertion. */
static void insert(DVM_OwnerScope *scope, DVM_OwnerEntry *entry,
    void *object, const DVM_OwnerOps *ops)
{
    entry->scope = scope;
    entry->object = object;
    entry->ops = ops;
    entry->next = scope->head;
    if (scope->head)
    {
        scope->head->previous = entry;
    }
    scope->head = entry;
}


/* Unlink and clear before retirement callback. entry is validated linked storage;
 * caller captures object/ops first. No object access or owner-count change. */
static void detach(DVM_OwnerEntry *entry)
{
    if (entry->previous)
    {
        entry->previous->next = entry->next;
    }
    else
    {
        entry->scope->head = entry->next;
    }
    if (entry->next)
    {
        entry->next->previous = entry->previous;
    }
    entry->scope = 0;
    entry->next = 0;
    entry->previous = 0;
    entry->object = 0;
    entry->ops = 0;
}


/* Registered native unwind callback. argument borrows still-live scope storage;
 * close unregisters it before returning. No throw, VM entry or new registration. */
static int unwind_scope(void *argument)
{
    return dvm_owner_scope_close(argument);
}


/* Bind private zeroed scope to current context. No roots are registered; callers
 * must close explicitly before unbinding. Return 0 with no partial open. */
int dvm_owner_scope_open(DVM_OwnerScope *scope)
{
    DVM_NativeContext *context = dvm_native_context_current();
    if (!scope || !context || scope->context || scope->head || scope->busy ||
        dvm_native_context_object_count(context) ||
        !dvm_native_cleanup_link(&scope->cleanup, unwind_scope, scope))
    {
        return 0;
    }
    scope->context = context;
    return 1;
}


/* Close owner-local scope; busy prevents callback reentry changing the ledger.
 * Clear entries before invoking drops, including final free. No allocation or
 * forced cancellation; callbacks must not throw/longjmp/change native context. */
int dvm_owner_scope_close(DVM_OwnerScope *scope)
{
    if (!available(scope))
    {
        return 0;
    }
    scope->busy = 1;
    while (scope->head)
    {
        DVM_OwnerEntry *entry = scope->head;
        void *object = entry->object;
        const DVM_OwnerOps *ops = entry->ops;
        detach(entry);
        ops->drop(object);
    }
    if (!dvm_native_cleanup_unlink(&scope->cleanup))
    {
        scope->busy = 0;
        return 0;
    }
    scope->context = 0;
    scope->busy = 0;
    return 1;
}


/* Retain while caller protects source lifetime; ledger mutation follows only a
 * successful callback. Reentry is refused until the new owner is fully linked. */
int dvm_owner_scope_retain(DVM_OwnerScope *scope, DVM_OwnerEntry *entry,
    void *object, const DVM_OwnerOps *ops)
{
    int retained;
    if (!available(scope) || !empty(entry) || !object || !ops || !ops->retain || !ops->drop)
    {
        return 0;
    }
    scope->busy = 1;
    retained = ops->retain(object);
    if (retained)
    {
        insert(scope, entry, object, ops);
    }
    scope->busy = 0;
    return retained != 0;
}


/* Consume linked entry only on its exact owner. Callback may free object; local
 * pointer is never dereferenced afterwards. Entry is empty during callback. */
int dvm_owner_scope_drop(DVM_OwnerEntry *entry)
{
    DVM_OwnerScope *scope;
    void *object;
    const DVM_OwnerOps *ops;
    if (!entry || !available(entry->scope))
    {
        return 0;
    }
    scope = entry->scope;
    object = entry->object;
    ops = entry->ops;
    scope->busy = 1;
    detach(entry);
    ops->drop(object);
    scope->busy = 0;
    return 1;
}


/* Move existing ownership into private empty envelope. Caller publishes it only
 * after success and never cancels while a receiver may access it. No callbacks. */
int dvm_owner_scope_export(DVM_OwnerEntry *entry, DVM_OwnerTransfer *transfer)
{
    if (!entry || !available(entry->scope) || !transfer || transfer->object || transfer->ops)
    {
        return 0;
    }
    transfer->object = entry->object;
    transfer->ops = entry->ops;
    detach(entry);
    return 1;
}


/* Receive exclusive envelope into own ledger. Valid transfer owns one reference;
 * adoption moves rather than duplicates it. All validation precedes mutation. */
int dvm_owner_scope_accept(DVM_OwnerScope *scope, DVM_OwnerEntry *entry,
    DVM_OwnerTransfer *transfer)
{
    if (!available(scope) || !empty(entry) || !transfer || !transfer->object ||
        !transfer->ops || !transfer->ops->retain || !transfer->ops->drop)
    {
        return 0;
    }
    insert(scope, entry, transfer->object, transfer->ops);
    transfer->object = 0;
    transfer->ops = 0;
    return 1;
}


/* Failed unpublished handoff still owns its reference. Clear the envelope
 * BEFORE callback; native caller must exclude concurrent accept/cancel. */
int dvm_owner_transfer_cancel(DVM_OwnerTransfer *transfer)
{
    DVM_NativeContext *context = dvm_native_context_current();
    void *object;
    const DVM_OwnerOps *ops;
    if (!context || dvm_native_context_object_count(context) || !transfer ||
        !transfer->object || !transfer->ops || !transfer->ops->drop)
    {
        return 0;
    }
    object = transfer->object;
    ops = transfer->ops;
    transfer->object = 0;
    transfer->ops = 0;
    ops->drop(object);
    return 1;
}
