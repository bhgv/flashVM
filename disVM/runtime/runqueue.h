/* Internal runnable FIFO and native-continuation ownership; include interp.h first. */
#ifndef DVM_RUNTIME_RUNQUEUE_H
#define DVM_RUNTIME_RUNQUEUE_H
#include "disvm/native_context.h"
#define DVM_RUNTIME_PROCESS_MAX 128 /* Shared process/runnable quota, including native waiters. */


/* Native allocation preserves the complete retained Prog prefix. Every Prog
 * passed to this module must come from this wrapper; free its prefix normally. */
typedef struct DVM_RuntimeProcess
{
    Prog process;                     /* Retained interpreter/GC-visible prefix. */
    int queued;                       /* O(1) membership, changed only by this queue module. */
    DVM_NativeContext *native_owner;   /* Borrowed continuation owner while off the runnable FIFO. */
} DVM_RuntimeProcess;


/* Borrow FIFO head under exclusive heap ownership; NULL means no runnable Prog. */
Prog *dvm_runqueue_head(void);


/* Append one unqueued, non-leased runtime Prog. No allocation; return zero for
 * duplicate/corrupt/full queues or an outstanding native continuation. */
int dvm_runqueue_append(Prog *process);


/* Remove the FIFO head and set its retained state. Return NULL on empty/invalid
 * queues. Caller retains the Prog in the separate GC/process list. */
Prog *dvm_runqueue_pop(int state);


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
