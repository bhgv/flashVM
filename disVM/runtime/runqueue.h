/* Copyright (c) 2026 bhgv
 * EN: Native readiness and permanent process/continuation ownership; include interp.h.
 * RU: Нативная готовность и постоянные владельцы процессов/ожиданий; нужен interp.h.
 */
#ifndef DVM_RUNTIME_RUNQUEUE_H
#define DVM_RUNTIME_RUNQUEUE_H
#include "disvm/native_context.h"
#include <fenv.h>
#include "owner_messages.h"
#include "channel_sync.h"
#ifndef DVM_RUNTIME_PROCESS_MAX
#define DVM_RUNTIME_PROCESS_MAX 2147483647U /* Native signed process/count representation, not a128-task policy. */
#endif


enum
{
    DVM_PROG_MESSAGE_CHANNEL_CLOSE = 1, /* The channel is retiring. */
    DVM_PROG_MESSAGE_CHANNEL_EXIT,      /* The guest was asked to exit. */
    DVM_PROG_MESSAGE_CHANNEL_CANCEL,    /* Host requested cancellation of this wait. */
    DVM_PROG_MESSAGE_CANCEL_WAIT,       /* Parent requests cancellation with a reply. */
    DVM_PROG_MESSAGE_CANCEL_RESULT,     /* Outcome of a parent cancellation request. */
    DVM_PROG_MESSAGE_CHILD_RETIRED      /* Child retirement reply to killwait. */
};


/* Native allocation preserves the complete retained Prog prefix. Every Prog
 * passed to this module must come from this wrapper; free its prefix normally. */
typedef struct DVM_RuntimeProcess
{
    Prog process;                      /* Retained interpreter/GC-visible prefix. */
    int exit_notify_parent;            /* A parent parked until this guest retires. */
    int exit_wait_pid;                 /* Child awaited by a prepared killwait frame. */
    int exit_ready_pid;                /* Retired child acknowledged by the next MCALL retry. */
    int parent_pid;                    /* Immutable numeric identity of the spawning guest. */
    int lifetime_holds;                /* Native borrows acquired under the heap turn. */
    DVM_MessageEndpoint *messages;     /* Dedicated guest registration, stable until retirement. */
    DVM_OwnerMessage channel_close;    /* Embedded close request, coalesced until wait completes. */
    DVM_OwnerMessage channel_exit;     /* Embedded exit request, coalesced until wait completes. */
    DVM_OwnerMessage channel_cancel;   /* Embedded host cancellation for the current wait. */
    uint64_t cancel_sequence;          /* Owner-issued operation ID; never reused during this lifetime. */
    uint64_t cancel_wait_request;      /* Active cancellation ID, protected by channel_queue_mutex. */
    int cancel_waiting;                /* This guest is parked for a cancellation reply. */
    int cancel_ready_pid;              /* Child matched by the prepared cancelwait retry. */
    int cancel_ready_result;           /* Result delivered to that retry. */
    int table_retry_pending;           /* Parked Bigtable MCALL, including after wake. */
    int table_cancel_pc;               /* Pending MCALL successor update for the TLS owner. */
    int cancel_delivering;             /* Owner is completing a posted cancellation. */
    DVM_OwnerMessage child_retired;    /* Embedded reply for the one parked killwait. */
    const void *channel_claimed;       /* Exclusive channel match under NativeWorld. */
    unsigned int arithmetic_retries;   /* Consecutive pre-effect retries of the current instruction. */
    struct ManagedWorker *dedicated;   /* Native wake slot, stable until guest retirement. */
    Prog *ready_previous;              /* Borrowed predecessor while ready. */
    const char *startup_error;         /* Stable registrar failure, read after its reply. */
    int startup_pending;               /* Atomic startup handshake; creator keeps the GC root live. */
    int queued;                        /* Queue membership; published atomically for the dedicated idle loop. */
    int observed_state;                /* Atomic state value for foreign request admission, never a resume image. */
    DVM_NativeContext *native_owner;   /* Borrowed continuation owner while off the runnable FIFO. */
    DVM_NativeContext *dispatch_owner; /* Atomic identity of the permanent managed turn, including native waits. */
    uintptr last_dispatcher; /* Numeric identity only, never dereferenced; live pool owners have distinct addresses. */
    Osenv environment;       /* Native guest error storage follows this logical Prog across owner migration. */
    fenv_t floating_environment; /* Guest FP environment retained across complete turns and OS owners. */
} DVM_RuntimeProcess;


/* EN: Set a borrowed process's execution state and publish its scalar observation.
 * Caller owns the transition or initializes unpublished storage; does not lock,
 * transfer ownership, publish other wait fields or restore registers. No failure.
 * RU: Установить состояние process и опубликовать отдельное скалярное наблюдение.
 * Caller владеет переходом либо инициализирует неопубликованную память. Без lock,
 * передачи владения, публикации иных полей ожидания или загрузки REG. Без ошибок. */
void dvm_runqueue_set_state(Prog *process, int state);


/* EN: Acquire only the scalar observation of a live borrowed process. The caller
 * keeps its registration/lifetime protected; other mutable fields remain guarded
 * separately. No blocking, ownership transfer or guarantee a wait still exists.
 * RU: Прочитать только скалярное наблюдение живого заимствованного process.
 * Caller защищает регистрацию/lifetime; прочие изменяемые поля защищены отдельно.
 * Без блокировки, передачи владения и гарантии, что ожидание ещё существует. */
int dvm_runqueue_observed_state(Prog *process);


/* EN: Consume owned CANCEL_WAIT storage when its wait is removed: free or
 * transfer the same node as a reply. Other kinds remain borrowed and unchanged.
 * RU: Принять память CANCEL_WAIT при снятии ожидания: освободить либо передать
 * тот же узел как ответ. Иные виды остаются заимствованными и неизменными. */
void dvm_runtime_wait_request_finished(Prog *guest, DVM_OwnerMessage *message);


/* EN: Enable direct dispatch before any ready Prog exists.
 * RU: Включить прямой запуск до появления готовых Prog. */
void dvm_runqueue_enable_direct(void);


/* EN: Publish this thread's prepared resident once, without entering the heap.
 * RU: Однократно опубликовать готовность своего процесса без входа в кучу. */
int dvm_runqueue_publish_startup(Prog *process);


/* EN: Report whether at least one Prog is ready.
 * RU: Проверить наличие хотя бы одного готового Prog. */
int dvm_runqueue_ready(void);


/* EN: Configure borrowed channel-publication sync before startup; NULL is the
 * standalone/reference mode. No caller may change it while native actors run.
 * RU: До запуска задать заимствованный sync публикации каналов; NULL выбирает
 * отдельный/эталонный режим. Менять sync при работающих потоках запрещено. */
void dvm_runqueue_ready_sync(FVM_PortSync *sync);


/* EN: Acquire-read the ready flag of independently live process, or zero for NULL.
 * It grants no process lifetime or mutable-payload access.
 * RU: С acquire прочитать готовность отдельно живого process; NULL даёт ноль.
 * Время жизни процесса и доступ к изменяемым данным этим не приобретаются. */
int dvm_runqueue_is_ready(Prog *process);


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


int dvm_runqueue_dispatch_pending(void);


/* Print actual queue/lease diagnostics after all native workers have joined. */
void dvm_runqueue_statistics(void);
#endif
