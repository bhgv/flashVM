/* Native object guards. They do not grant access to a managed Heap. */
#ifndef DVM_OBJECT_GUARD_H
#define DVM_OBJECT_GUARD_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DVM_ObjectDomain DVM_ObjectDomain; /* Native parking table, retained by initialized guards. */


/* Inline native metadata; zero before init, then treat fields as private.
 * Keep its address stable until fini: initialized guards are not copyable or
 * movable. Caller holds object storage alive through every operation/wait. No GC roots
 * are inferred here. This is NOT a change to the retained Heap header layout. */
typedef struct DVM_ObjectGuard
{
    DVM_ObjectDomain *domain; /* Immutable between successful init/fini. */
    void *owner;             /* Atomic context identity; never dereference or write directly. */
    unsigned long waiters;   /* Parking predicate users, protected by the domain stripe. */
} DVM_ObjectGuard;


/* Distinguish availability notification from actual exclusive acquisition. */
typedef enum DVM_ObjectResult
{
    DVM_OBJECT_REFUSED = -1, /* Invalid role, recursive acquisition or exhausted native metadata. */
    DVM_OBJECT_BUSY = 0,     /* Try-enter could not claim; no ownership was acquired. */
    DVM_OBJECT_ACQUIRED = 1, /* Current bound context owns this guard. */
    DVM_OBJECT_AVAILABLE = 2 /* Wait observed availability; retry try-enter, no reservation granted. */
} DVM_ObjectResult;


/* Allocate a native domain and fixed-size shared parking table. No VM access.
 * Return owned domain or NULL with partial native allocations cleaned up. */
DVM_ObjectDomain *dvm_object_domain_create(void);


/* Caller must quiesce all new users. Refuse without mutation while any guard
 * remains initialized. Successful destruction consumes the caller's domain. */
int dvm_object_domain_destroy(DVM_ObjectDomain *domain);


/* Initialize a private zeroed guard and retain domain until fini. Return1 on
 * success,0 on invalid/exhausted state. No object/GC ownership is acquired. */
int dvm_object_guard_init(DVM_ObjectGuard *guard, DVM_ObjectDomain *domain);


/* Finalize only quiescent unowned guard storage. Refuse held/waited guards;
 * caller must already exclude new operations through object lifetime ownership. */
int dvm_object_guard_fini(DVM_ObjectGuard *guard);


/* Bounded native spin/CAS on an already-live guard. Requires bound TLS context;
 * allow bare native contexts or acknowledged ACTIVE VM owners only. Recursive,
 * idle/native/collector use refuses. Does not wait for object availability or
 * allocate. The portable metadata fallback can take the port registry mutex;
 * do not call it while already holding that mutex. */
DVM_ObjectResult dvm_object_guard_try_enter(DVM_ObjectGuard *guard);


/* Park without taking ownership. Caller retains object and publishes any VM
 * roots BEFORE leaving its heap interval. ACTIVE/collector or held-guard callers
 * refuse. AVAILABLE is only a notification; another owner can win the retry.
 * No forced OS cancellation; no managed payload access or callback occurs. */
DVM_ObjectResult dvm_object_guard_wait_available(DVM_ObjectGuard *guard);


/* Release only from the exact current owner, then wake predicate waiters. No
 * ownership transfer across contexts. Caller retains object through return. */
int dvm_object_guard_leave(DVM_ObjectGuard *guard);


/* Copy native waiter count under its stripe lock; no lifetime/root extension.
 * Caller retains initialized guard/domain and must not use this as permission. */
unsigned long dvm_object_guard_waiters(DVM_ObjectGuard *guard);


/* Report native word implementation, not a wire/profile/layout version. */
const char *dvm_object_guard_backend(void);
#ifdef __cplusplus
}
#endif
#endif
