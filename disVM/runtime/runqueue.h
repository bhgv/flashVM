/* Internal runnable FIFO and native-continuation ownership; include interp.h first. */
#ifndef DVM_RUNTIME_RUNQUEUE_H
#define DVM_RUNTIME_RUNQUEUE_H
#include "disvm/native_context.h"
#include <fenv.h>
#ifndef DVM_RUNTIME_PROCESS_MAX
#define DVM_RUNTIME_PROCESS_MAX 2147483647U /* Native signed process/count representation, not a128-task policy. */
#endif


/* Native allocation preserves the complete retained Prog prefix. Every Prog
 * passed to this module must come from this wrapper; free its prefix normally. */
typedef struct DVM_RuntimeProcess
{
    Prog process;                     /* Retained interpreter/GC-visible prefix. */
    int exit_notify_parent;           /* A parent parked until this guest retires. */
    int exit_wait_pid;                /* Child awaited by a prepared killwait frame. */
    int exit_ready_pid;               /* Retired child acknowledged by the next MCALL retry. */
    int parent_pid;                   /* Immutable numeric identity of the spawning guest. */
    int lifetime_holds;      /* Native borrows acquired under the heap turn. */
    const void *channel_claimed;              /* Exclusive channel match under NativeWorld. */
    unsigned int arithmetic_retries;  /* Consecutive pre-effect retries of the current instruction. */
    struct ManagedWorker *dedicated;   /* Native wake slot, stable until guest retirement. */
    Prog *ready_previous;             /* Borrowed predecessor while ready. */
    int queued;                       /* O(1) membership, changed only by this queue module. */
    DVM_NativeContext *native_owner;   /* Borrowed continuation owner while off the runnable FIFO. */
    DVM_NativeContext *dispatch_owner; /* Borrowed owner of the entire managed turn, including native waits. */
    uintptr last_dispatcher;           /* Numeric identity only, never dereferenced; live pool owners have distinct addresses. */
    Osenv environment;                /* Native guest error storage follows this logical Prog across owner migration. */
    fenv_t floating_environment;       /* Guest FP environment retained across complete turns and OS owners. */
} DVM_RuntimeProcess;


/* Pin a scheduler-owned Prog under the heap turn. Each successful pin must
 * be released before that Prog can retire. The caller already owns its input. */
int dvm_runqueue_pin(Prog *process);
void dvm_runqueue_unpin(Prog *process);
int dvm_runqueue_hold_count(Prog *process);
int dvm_runqueue_begin_retire(Prog *process);


/* Claim only the runnable head on the currently bound, idle native context.
 * Caller holds the exclusive heap turn. Publishes current process on success;
 * duplicate/foreign/nested claims refuse without queue/root changes. No throw. */
int dvm_runqueue_claim(Prog *process);


/* Enable bounded native-wait peak diagnostics before launching managed owners.
 * No heap access; one immutable launch setting, used only by explicit --stats. */
void dvm_runqueue_trace_waits(void);


/* End this owner's complete managed turn after register/root publication and
 * native reentry, before retirement or rotation. Process may already be blocked.
 * Clears current-process identity if still present. A foreign/live-native lease
 * cannot finish. Caller holds heap turn and keeps process allocated throughout. */
int dvm_runqueue_finish(Prog *process);


/* Observe a live process's managed-turn owner under heap exclusion; NULL means
 * unclaimed. This does not transfer ownership or access another context's TLS. */
DVM_NativeContext *dvm_runqueue_dispatch_owner(Prog *process);


/* Borrow FIFO head under exclusive heap ownership; NULL means no runnable Prog. */
Prog *dvm_runqueue_head(void);

/* Bring a dedicated thread's ready guest to the retained dispatch head. */
Prog *dvm_runqueue_select_guest(Prog *process);


/* Append one unqueued, non-leased runtime Prog. No allocation; return zero for
 * duplicate/corrupt/full queues or an outstanding native continuation. */
int dvm_runqueue_append(Prog *process);


/* Publish a detached waiter with cleared wait fields and optional borrowed
 * cause. Return zero unchanged on refusal. Caller owns NativeWorld. */
int dvm_runqueue_complete_channel_wait(Prog *process, char *cause);


/* Remove the FIFO head and set its retained state. Return NULL on empty/invalid
 * queues. Caller retains the Prog in the separate GC/process list. */
Prog *dvm_runqueue_pop(int state);

/* Withdraw the expected live guest only while it is the FIFO head. */
Prog *dvm_runqueue_pop_expected(Prog *expected, int state);


/* Inspect the retained synchronous Native64Caller hold while heap ownership is
 * active. NULL has no program hold. This never promotes that lease to resumable. */
int dvm_runqueue_release_allowed(Prog *process);


/* [GC:] Withdraw the current head before releasing heap ownership. Current TLS
 * must own this Prog; caller publishes REG first and keeps the Prog rooted/live.
 * Success marks Prelease and pins its C continuation to this native context. */
int dvm_runqueue_suspend(Prog *process);


/* Restore only this continuation's original native owner after heap reentry.
 * Prepend to satisfy retained current-head operations; no other worker may run
 * this Prog while leased. Wrong owner/state/duplicate reentry returns zero. */
int dvm_runqueue_resume(Prog *process);


/* Inspect queue membership under the heap turn: one/zero, or -1 on corruption.
 * process must be a live native runtime Prog; this grants no execution rights. */
int dvm_runqueue_contains(Prog *process);


/* Borrow a live runtime Prog's continuation owner under the heap turn, or NULL.
 * This is native identity, not a root/reference or cross-thread TLS accessor. */
DVM_NativeContext *dvm_runqueue_owner(Prog *process);


/* Copy the outstanding native-lease count under exclusive heap ownership.
 * No heap/root changes or implicit scheduling; used after all owners return. */
unsigned long dvm_runqueue_inflight(void);


/* Print actual queue/lease diagnostics after all native workers have joined. */
void dvm_runqueue_statistics(void);
#endif
