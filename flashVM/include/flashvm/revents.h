/* Resource discovery and independent copied event queues; no implicit network transport. */
#ifndef FLASHVM_REVENTS_H
#define FLASHVM_REVENTS_H /* Guard the additive C89 observer API. */
#include "rdispatch.h"
#define FVM_R_WATCHES 16U /* Maximum independent subscriptions per dispatcher. */
#define FVM_R_EVENT_QUEUE                                                                          \
    8U /* Retained events for each subscription; oldest is dropped on overflow. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* A subscription incarnation is distinct from its observed resource incarnation. */
    typedef struct FVM_RWatch
    {
        unsigned long id;         /* One-based subscription slot, scoped to this dispatcher. */
        unsigned long generation; /* Nonzero Uint32 generation; exhausted slots are retired. */
    } FVM_RWatch;
    /* A native copy; encode fields explicitly before IPC/network use. */
    typedef struct FVM_REvent
    {
        FVM_RHandle target;     /* Resource generation at publication. */
        unsigned long sequence; /* Dispatcher-wide monotonic publication sequence; never wraps. */
        unsigned long kind;     /* Nonzero Uint32 resource-specific event schema/operation. */
        unsigned long
            missed;  /* Events dropped for this reader since last take; saturates at Uint32 max. */
        size_t size; /* Active payload bytes, bounded by FVM_R_BYTES. */
        unsigned char
            data[FVM_R_BYTES]; /* Copied payload, never a VM pointer or borrowed storage. */
    } FVM_REvent;
    /* One discoverable resource; operation metadata is filtered to current granted rights. */
    typedef struct FVM_RResource
    {
        FVM_RHandle target;        /* Exact registered incarnation, invalid after unregister. */
        char path[FVM_R_PATH + 1]; /* Copied canonical absolute resource name. */
        unsigned int rights;       /* Current rights for the authenticated caller session. */
        size_t count;              /* Number of authorized operation descriptions below. */
        FVM_ROperation operations[FVM_R_OPERATIONS]; /* Copied server-defined IDs and rights. */
    } FVM_RResource;
    /* Coherent discovery under the dispatcher lock. NULL rows with capacity zero queries count.
     * Otherwise LIMIT leaves rows/count unchanged. Invisible resources are omitted. No directory,
     * subtree inheritance, filesystem mount or ambient OS lookup is implied. */
    FVM_API FVM_RResult fvm_rdispatch_resources(FVM_RDispatch *, unsigned long session,
                                                FVM_RResource *rows, size_t capacity,
                                                size_t *count);
    /* Return the immutable zero-based server operation position for a currently authorized ID.
     * Filesystem adapters use it for stable qids across sessions and rights-filter changes.
     * UNKNOWN resource/operation, DENIED missing right; output changes only on OK. */
    FVM_API FVM_RResult fvm_rdispatch_operation_index(FVM_RDispatch *, unsigned long session,
                                                      FVM_RHandle, unsigned long operation,
                                                      size_t *index);
    /* Any synchronized producer may subscribe when currently granted SUBSCRIBE. Each successful
     * call creates an independent empty queue; output changes only on OK. */
    FVM_API FVM_RResult fvm_rdispatch_subscribe(FVM_RDispatch *, unsigned long session,
                                                FVM_RHandle target, FVM_RWatch *);
    /* Owner copies one event to every current subscriber. Overflow drops each slow reader's oldest
     * event independently and counts loss. Sequence exhaustion returns LIMIT without publication.
     * No callbacks, allocation, serialization, blocking I/O or delivery acknowledgement occurs. */
    FVM_API FVM_RResult fvm_rdispatch_publish(FVM_RDispatch *, FVM_RHandle target,
                                              unsigned long kind, const void *, size_t,
                                              unsigned long *sequence);
    /* Copy/dequeue one event for its authenticated session. PENDING/errors leave output unchanged.
     * Revoking SUBSCRIBE or unregistering destroys affected queues immediately; old watches
     * UNKNOWN. A wrong session cannot consume another reader's events. No managed objects/GC work
     * occurs. */
    FVM_API FVM_RResult fvm_rdispatch_event_take(FVM_RDispatch *, unsigned long session, FVM_RWatch,
                                                 FVM_REvent *);
    /* Check an existing observer without consuming data. Revoked/recreated subscriptions UNKNOWN;
     * wrong session DENIED. Adapters use this before returning partially buffered event bytes. */
    FVM_API FVM_RResult fvm_rdispatch_watch_check(FVM_RDispatch *, unsigned long session,
                                                  FVM_RWatch);
    /* Authenticated consumer closes its own queue; stale incarnation UNKNOWN, other session DENIED.
     */
    FVM_API FVM_RResult fvm_rdispatch_unsubscribe(FVM_RDispatch *, unsigned long session,
                                                  FVM_RWatch);
    /* Owner disconnect atomically removes grants, subscriptions, queued jobs and retained replies.
     * A running request for that session returns BUSY with no changes. Quiesce its transport first;
     * the adapter must reject old packets and allocate a fresh session/epoch on reconnection.
     * Other sessions' queue order is preserved; completed side effects are never rolled back. */
    FVM_API FVM_RResult fvm_rdispatch_disconnect(FVM_RDispatch *, unsigned long session);
#ifdef __cplusplus
}
#endif
#endif
