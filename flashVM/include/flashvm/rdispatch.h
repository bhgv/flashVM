/* Bounded native resource dispatcher: copied requests, owner execution and per-session grants. */
#ifndef FLASHVM_RDISPATCH_H
#define FLASHVM_RDISPATCH_H /* Guard the C89 resource-control ABI. */
#include "srecord.h"
#define FVM_R_RESOURCES 16U /* Maximum live/retired resource slots in one dispatcher. */
#define FVM_R_REQUESTS 32U  /* Queued/running/completed requests share this bounded capacity. */
#define FVM_R_GRANTS 64U    /* Maximum explicit session/resource permission entries. */
#define FVM_R_OPERATIONS 8U /* Immutable operation descriptions copied for each resource. */
#define FVM_R_BYTES 512U    /* Control-plane request/reply payload bound; not a media buffer. */
#define FVM_R_PATH 95U      /* Maximum bytewise canonical resource-path length, excluding NUL. */
#define FVM_R_READ 1U       /* Read state operation authority. */
#define FVM_R_WRITE 2U      /* Change state operation authority. */
#define FVM_R_CONTROL 4U    /* Timeline/device command authority. */
#define FVM_R_SUBSCRIBE                                                                            \
    8U                   /* Subscribe operation authority, not an implemented event transport.     \
                          */
#define FVM_R_RIGHTS 15U /* All currently defined authority bits. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Native statuses distinguish queue acceptance from eventual handler result. */
    typedef enum FVM_RResult
    {
        FVM_R_OK = 0,       /* Accepted, copied, or successfully completed operation. */
        FVM_R_INVALID = 1,  /* Invalid descriptor/value/handler output. */
        FVM_R_LIMIT = 2,    /* Bounded resources, grants, requests or payload capacity exhausted. */
        FVM_R_UNKNOWN = 3,  /* Unknown resource generation, operation or request. */
        FVM_R_DENIED = 4,   /* Wrong owner or missing current session authority. */
        FVM_R_CONFLICT = 5, /* Duplicate name/request identity or already finished cancellation. */
        FVM_R_PENDING = 6,  /* Result not ready; output remains untouched. */
        FVM_R_BUSY = 7, /* Executing/pumping resource cannot be destroyed/cancelled/reentered. */
        FVM_R_CANCELLED = 8, /* Queued command cancelled before execution. */
        FVM_R_NOMEM = 9,     /* Dispatcher allocation failed; caller outputs unchanged. */
        FVM_R_HANDLER =
            10 /* Handler returned an invalid status/extent; side effects cannot roll back. */
    } FVM_RResult;
    /* Resource identity is local to this dispatcher lifetime, never an authority token by itself.
     */
    typedef struct FVM_RHandle
    {
        unsigned long id; /* One-based bounded slot ID. */
        unsigned long
            generation; /* Nonzero 32-bit generation; never wraps/reuses an old generation. */
    } FVM_RHandle;
    /* Server-defined operation IDs are independent of direction-local S-record type tags. */
    typedef struct FVM_ROperation
    {
        unsigned long id;   /* Nonzero unsigned 32-bit service operation ID. */
        unsigned int right; /* Exactly one READ/WRITE/CONTROL/SUBSCRIBE bit, chosen by server. */
    } FVM_ROperation;
    /* Post borrows data only until return; the dispatcher copies active payload bytes. */
    typedef struct FVM_RRequest
    {
        unsigned long session; /* Authenticated adapter-assigned nonzero 32-bit session, not trusted
                                  wire input. */
        unsigned long
            id; /* Nonzero 32-bit request identity, unique until result acknowledgement. */
        FVM_RHandle target;      /* Generation checked at post and execution. */
        unsigned long operation; /* Resolved against the immutable server operation table. */
        const void *data; /* Borrowed on post; handler receives a copied dispatcher-owned borrow. */
        size_t size;      /* Active bytes, at most BYTES. */
    } FVM_RRequest;
    /* A copied result, not a borrowed VM pointer. Native structs must never be written directly on
     * wire. */
    typedef struct FVM_RReply
    {
        FVM_RResult result; /* Owner-handler result, not a general distributed commit guarantee. */
        size_t size;        /* Active reply bytes. */
        unsigned char
            data[FVM_R_BYTES]; /* Copied result storage independent from queue lifetime. */
    } FVM_RReply;
    /* Owner callback outside queue lock. Initialize *used and write <=capacity bytes; return a
     * defined result. No exception/longjmp across C. It may post/query/cancel other requests, but
     * not recurse pump or destroy/unregister this executing resource. Userdata remains owned by the
     * host. */
    typedef FVM_RResult (*FVM_RHandler)(const FVM_RRequest *, void *reply, size_t capacity,
                                        size_t *used, void *userdata);
    /* Trusted host policy callbacks must not reenter the dispatcher themselves. */
    typedef struct FVM_RPolicy
    {
        size_t struct_size; /* Exact native size; zero-init then fill before create. */
        void (*lock)(
            void *); /* Both lock/unlock or neither; real shared use requires a working mutex. */
        void (*unlock)(void *); /* Release the same host-owned lock; assumed infallible. */
        int (*is_owner)(
            void *); /* Optional owner-thread check for mutation/pump/destroy, never post/take. */
        void *userdata; /* Borrowed policy state/mutex valid through successful destroy. */
    } FVM_RPolicy;
    typedef struct FVM_RDispatch
        FVM_RDispatch; /* Opaque native allocation with no managed objects. */
    /* Create validates/copies policy and allocates one fixed-size dispatcher; output changes only
     * on OK. NULL policy is single-owner unsynchronized mode. Handles/IDs cannot cross dispatcher
     * lifetimes. */
    FVM_API FVM_RResult fvm_rdispatch_create(const FVM_RPolicy *, FVM_RDispatch **);
    /* Trusted owner registers a copied canonical absolute path and operation table; copies neither
     * handler userdata nor managed objects. Outputs unchanged on failure; duplicate paths CONFLICT.
     */
    FVM_API FVM_RResult fvm_rdispatch_register(FVM_RDispatch *, const char *path,
                                               const FVM_ROperation *, size_t count, FVM_RHandler,
                                               void *userdata, FVM_RHandle *);
    /* Owner releases registration/grants, not userdata. Running handler BUSY; queued old
     * generations subsequently complete UNKNOWN. Generation exhaustion retires the slot without
     * wrapping. */
    FVM_API FVM_RResult fvm_rdispatch_unregister(FVM_RDispatch *, FVM_RHandle);
    /* Owner creates/replaces authority; rights=0 revokes. Rechecked at execution; running callbacks
     * are not interrupted. Session must be authenticated by adapter before it reaches this API. */
    FVM_API FVM_RResult fvm_rdispatch_grant(FVM_RDispatch *, FVM_RHandle, unsigned long session,
                                            unsigned int rights);
    /* Find an exact canonical path only if session has any authority there; output only on OK. */
    FVM_API FVM_RResult fvm_rdispatch_lookup(FVM_RDispatch *, unsigned long session,
                                             const char *path, FVM_RHandle *);
    /* Any producer may enqueue with working lock hooks. Full/duplicate/rejected posts cause no
     * effects. Identity retention is bounded until take: retries after take may execute again. */
    FVM_API FVM_RResult fvm_rdispatch_post(FVM_RDispatch *, const FVM_RRequest *);
    /* Owner executes at most budget (1..REQUESTS) FIFO jobs. Output count set on OK; reentry BUSY.
     * Handler failures are reply results, not a pump failure. Userdata is pinned by native
     * registration lifetime only: VM handlers must arrange owner-side managed roots explicitly. */
    FVM_API FVM_RResult fvm_rdispatch_pump(FVM_RDispatch *, size_t budget, size_t *completed);
    /* Cancel only queued work for this session/ID; running BUSY, already done CONFLICT. */
    FVM_API FVM_RResult fvm_rdispatch_cancel(FVM_RDispatch *, unsigned long session,
                                             unsigned long id);
    /* Take copies/removes a completed reply, acknowledging identity and freeing queue capacity.
     * PENDING/UNKNOWN/error leaves output untouched. Caller supplies session identity via trusted
     * adapter. */
    FVM_API FVM_RResult fvm_rdispatch_take(FVM_RDispatch *, unsigned long session, unsigned long id,
                                           FVM_RReply *);
    /* Owner destroy requires producers/consumers quiesced and every accepted job taken; otherwise
     * BUSY. Frees only dispatcher, never host userdata/mutex/VM resources; sets *dispatcher=NULL on
     * OK. */
    FVM_API FVM_RResult fvm_rdispatch_destroy(FVM_RDispatch **);
#ifdef __cplusplus
}
#endif
#endif
