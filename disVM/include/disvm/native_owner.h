/* Native heap-turn and audited-call state owned by one bound TLS context. */
#ifndef DISVM_NATIVE_OWNER_H
#define DISVM_NATIVE_OWNER_H
#include "native_world.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DVM_HeapOwner DVM_HeapOwner; /* Context-owned native metadata, never a managed root/payload. */


/* Explicit lifecycle; only the bound owner changes its private state. */
typedef enum DVM_HeapPhase
{
    DVM_HEAP_DETACHED = 0, /* No registered root set. */
    DVM_HEAP_IDLE,         /* Registered immutable roots; no heap access. */
    DVM_HEAP_ACTIVE,       /* Holds the exclusive heap turn. */
    DVM_HEAP_NATIVE,       /* Audited native wait; roots remain immutable. */
    DVM_HEAP_SHARED        /* Restricted work; ordinary heap operations refuse. */
} DVM_HeapPhase;


/* Owner-only observation; counters may wrap and do not describe other owners. */
typedef struct DVM_HeapOwnerStats
{
    DVM_HeapPhase phase;          /* Current private lifecycle state. */
    unsigned long releases;      /* Successful audited native suspensions. */
    unsigned long reentries;     /* Matching native resumptions. */
    unsigned long gc_progress;   /* Summed caller-sampled epochs during native waits. */
    int pending_progress;        /* Resume completed; epoch must be sampled before another transition. */
} DVM_HeapOwnerStats;


/* Borrow zero-initialized native metadata embedded in current TLS context.
 * NULL means no context is bound. Its lifetime ends with context destruction;
 * it contains no managed ownership and is not available on another thread. */
DVM_HeapOwner *dvm_native_owner_current(void);


/* [GC:] Register current complete roots in a live serialized world. owner must
 * be current, detached metadata; world stays alive through detach. Returns zero
 * on misuse/allocation/stop conflict without admitting heap access. Success is
 * IDLE; caller must enter before reading/writing managed memory. */
int dvm_native_owner_attach(DVM_HeapOwner *owner, DVM_NativeWorld *world);


/* Enter an IDLE owner's FIFO heap turn. Borrow owner; may block for peers/GC.
 * Return zero for wrong phase/foreign owner without consuming an admission. */
int dvm_native_owner_enter(DVM_HeapOwner *owner);


/* [GC:] Publish stable roots and leave ACTIVE for IDLE. No heap access afterward;
 * no native-call token is created. Pending progress/foreign owners are refused. */
int dvm_native_owner_leave(DVM_HeapOwner *owner);


/* [GC:] Leave ACTIVE for audited NATIVE work. epoch is sampled while ACTIVE;
 * caller first publishes its managed frame and external roots and satisfies
 * higher-level synchronous-call restrictions. This does not convert a retained
 * Native64Caller hold into a releasing lease. Native work must
 * not access managed data. Returns zero on misuse, leaving state unchanged. */
int dvm_native_owner_suspend(DVM_HeapOwner *owner, unsigned long epoch);


/* Enter restricted shared admission from NATIVE on the same owner. All
 * operand owners and stable roots must already be published. The shared
 * body must use its own protection, avoid VM callbacks and release guards
 * before leaving. These transitions do not consume the native resume token. */
int dvm_native_owner_enter_shared(DVM_HeapOwner *owner);
int dvm_native_owner_leave_shared(DVM_HeapOwner *owner);


/* Reacquire the FIFO turn for this NATIVE owner. May block; successful return
 * permits sampling the shared collector epoch and completing progress exactly
 * once before another lifecycle transition. No cross-thread token transfer. */
int dvm_native_owner_resume(DVM_HeapOwner *owner);

/* Resume the suspended continuation directly from its shared interval. */
int dvm_native_owner_resume_shared(DVM_HeapOwner *owner);


/* Record epoch AFTER successful resume, under active heap ownership. Returns
 * zero without change unless one matching progress sample is outstanding.
 * Unsigned epoch subtraction supports the collector counter's ordinary wrap. */
int dvm_native_owner_progress(DVM_HeapOwner *owner, unsigned long epoch);


/* Close new root registration only when this ACTIVE owner is the world's last
 * registration. Peers/foreign/pending-progress refuse without closing the world.
 * Existing last-owner operations remain valid until leave/detach. */
int dvm_native_owner_seal(DVM_HeapOwner *owner);


/* Remove IDLE roots and reset native metadata. Borrow owner/world; no managed
 * ownership is released here. Foreign/active/native/pending states refuse. */
int dvm_native_owner_detach(DVM_HeapOwner *owner);


/* Borrow the attached world on the bound owner, or NULL for foreign/detached.
 * This is metadata, not permission for heap access or a lifetime reservation. */
DVM_NativeWorld *dvm_native_owner_world(DVM_HeapOwner *owner);


/* Copy current attached owner's fast/locked poll counters into borrowed native
 * outputs. No cross-owner read, reservation or heap access; detached fails. */
int dvm_native_owner_polls(DVM_HeapOwner *owner, size_t *fast, size_t *locked);


/* Copy private state into caller-owned out on the same bound OS owner. Also
 * works while NATIVE/IDLE; touches no published root storage. Foreign/null fail. */
int dvm_native_owner_statistics(DVM_HeapOwner *owner, DVM_HeapOwnerStats *out);
#ifdef __cplusplus
}
#endif
#endif
