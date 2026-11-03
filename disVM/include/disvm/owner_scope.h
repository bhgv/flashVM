/* Explicit native strong owners; no managed roots or payload permissions. */
#ifndef DVM_OWNER_SCOPE_H
#define DVM_OWNER_SCOPE_H
#include "disvm/native_context.h"
#include "disvm/native_cleanup.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DVM_OwnerScope DVM_OwnerScope; /* Noncopyable owner-local reference ledger. */
typedef struct DVM_OwnerEntry DVM_OwnerEntry; /* Stable caller-owned slot, alive until dropped/closed. */


/* Immutable borrowed callbacks, alive through every scope and transfer using
 * them. retain(object) returns nonzero for exactly one new strong owner, zero
 * without mutation on refusal. drop(object) consumes exactly one strong owner,
 * including final retirement. Both must return normally, not call the VM, switch
 * context, mutate this ledger or wait for managed access. No GC roots implied. */
typedef struct DVM_OwnerOps
{
    int (*retain)(void *object); /* Caller already protects object lifetime before retain. */
    void (*drop)(void *object); /* May free object; ledger never touches it afterwards. */
} DVM_OwnerOps;


/* Zero before open. Fields private; owner must close before TLS unbind/context
 * destruction and keep every entry's storage live. Hosted native checkpoints may
 * close newer scopes BEFORE longjmp; arbitrary C/OS unwinding is unsupported. */
struct DVM_OwnerScope
{
    DVM_NativeCleanup cleanup; /* Context registration, unwound before abandoning native frames. */
    DVM_NativeContext *context; /* Borrowed bound context, fixed until close. */
    DVM_OwnerEntry *head;       /* LIFO owned references, not heap allocations. */
    int busy;                  /* Refuse synchronous callback reentry. */
};


/* Zero before insertion; never copy/move a linked entry. A local alias in C is
 * not another entry unless explicitly retained. Payload access is separate. */
struct DVM_OwnerEntry
{
    DVM_OwnerScope *scope;    /* Owning ledger, or NULL for an empty slot. */
    DVM_OwnerEntry *next;     /* Intrusive next owner; stable caller storage. */
    DVM_OwnerEntry *previous; /* Intrusive previous owner, or NULL at head. */
    void *object;            /* Exactly one strong native owner while linked. */
    const DVM_OwnerOps *ops;  /* Borrowed immutable lifetime callbacks. */
};


/* Zeroed, noncopyable one-owner envelope. Caller synchronizes publication and
 * excludes concurrent take/cancel; no raw cross-thread ledger access allowed. */
typedef struct DVM_OwnerTransfer
{
    void *object;           /* Exactly one owned reference, or NULL when empty. */
    const DVM_OwnerOps *ops; /* Borrowed immutable callbacks, not serialized ABI. */
} DVM_OwnerTransfer;


/* Open private zeroed scope on current bound context. scope borrowed; return
 * 1 on success, 0 without mutation on invalid state/held object guards. */
int dvm_owner_scope_open(DVM_OwnerScope *scope);


/* Consume all entries in reverse insertion order, clearing each before drop.
 * scope and entries remain caller-owned. Return 0 unchanged for foreign/context/
 * held-guard/reentrant use; success empties scope and returns 1. No allocation. */
int dvm_owner_scope_close(DVM_OwnerScope *scope);


/* Check an empty destination in current bound scope without mutation. Return0
 * for foreign/busy/guarded/closed scope or nonempty/invalid entry. Caller owns
 * stable entry storage. No reservation survives a callback/context transition;
 * actual insertion must validate again. No owner/root/payload rights acquired. */
int dvm_owner_scope_can_accept(DVM_OwnerScope *scope, DVM_OwnerEntry *entry);


/* Retain one DISTINCT owner into empty entry in current scope. object is already
 * protected by caller; callbacks are borrowed. Return 0 unchanged on refusal.
 * No allocation; not permission to read/write shared payload without its guard. */
int dvm_owner_scope_retain(DVM_OwnerScope *scope, DVM_OwnerEntry *entry,
    void *object, const DVM_OwnerOps *ops);


/* Consume one entry as if assigning nil. Unlink/clear precedes drop callback;
 * exact current scope owner only. Return 0 unchanged on invalid role/state. */
int dvm_owner_scope_drop(DVM_OwnerEntry *entry);


/* Move entry's existing owner into empty transfer without retaining/dropping.
 * Owner-local source only, transfer privately borrowed. Publish only AFTER this
 * succeeds. Return 0 unchanged on refusal; scopes never cross threads. */
int dvm_owner_scope_export(DVM_OwnerEntry *entry, DVM_OwnerTransfer *transfer);


/* Consume exclusively received transfer into empty current-owner entry. No
 * callback/count change; success clears transfer. Refusal changes neither.
 * Caller must use a real native synchronization boundary to receive envelope. */
int dvm_owner_scope_accept(DVM_OwnerScope *scope, DVM_OwnerEntry *entry,
    DVM_OwnerTransfer *transfer);


/* Exchange a linked current-owner entry with a POPULATED exclusive transfer.
 * Both owners remain accounted for; entry stays linked in the same scope.
 * No retain/drop/callback/allocation/wait. Return0 unchanged on foreign/busy/
 * guarded caller, empty/malformed pair. Storage must be disjoint, privately
 * owned or externally locked; never publish the transfer without synchronization. */
int dvm_owner_scope_exchange(DVM_OwnerEntry *entry, DVM_OwnerTransfer *transfer);


/* Consume exclusively owned unpublished/returned transfer, clearing it before
 * drop. Use for failed thread-start rollback; not to cancel an in-flight owner.
 * Return 0 unchanged when empty/invalid, guarded or without bound context. */
int dvm_owner_transfer_cancel(DVM_OwnerTransfer *transfer);
#ifdef __cplusplus
}
#endif
#endif
