/* Operation-owned blocking guards for host payloads. POSIX implementation. */
#ifndef FVM_OWNED_H
#define FVM_OWNED_H
#include <stdint.h>
#include <pthread.h>

typedef struct
{
    pthread_mutex_t metadata; /* Protects ownership, never held during payload work. */
    pthread_cond_t changed;   /* Parks competing operations without spinning. */
    uint64_t owner;           /* Nonzero live operation capability, zero when idle. */
    unsigned depth;           /* Balanced entries by the current operation. */
    unsigned waiters;         /* Diagnostic count; not a cancellation mechanism. */
} FvmOwned;

/* Initialize caller-owned, unpublished storage. Returns POSIX error or zero.
 * No payload is owned. The caller must not copy an initialized guard. */
int fvm_owned_init(FvmOwned *guard);


/* Enter with nonzero operation token; block until idle or recursively owned.
 * Returns POSIX error or zero. One token MUST have one executing continuation;
 * callers cannot concurrently execute the same token on distinct native threads.
 * Caller must disable pthread cancellation across enter/work/leave; otherwise
 * returns EINVAL. Use cooperative cancellation with balanced leave instead. */
int fvm_owned_enter(FvmOwned *guard, uint64_t token);


/* Enter immediately or return EBUSY without changing ownership/waiter counts.
 * Borrowed initialized guard, nonzero token and disabled cancellation required.
 * Only the short metadata mutex may wait; no payload wait or spin occurs. */
int fvm_owned_try_enter(FvmOwned *guard, uint64_t token);


/* Leave exactly one entry. Wrong/zero token returns EPERM without mutation.
 * Payload writes must finish before the final leave; timeout is not completion. */
int fvm_owned_leave(FvmOwned *guard, uint64_t token);


/* Snapshot metadata into nonnull caller outputs. Never acquires the payload.
 * Returns POSIX error or zero; diagnostics remain available while owner pauses. */
int fvm_owned_inspect(FvmOwned *guard, uint64_t *owner, unsigned *depth, unsigned *waiters);


/* Destroy a quiescent, externally unpublished guard. EBUSY if owned or awaited.
 * Caller must prevent new entries throughout destruction and release storage. */
int fvm_owned_destroy(FvmOwned *guard);
#endif
