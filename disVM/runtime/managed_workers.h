/* Copyright (c) 2026 bhgv
 * EN: Lifecycle of native execution threads permanently assigned to one Prog.
 * RU: Жизненный цикл нативных потоков, закреплённых за одним Prog.
 */
#ifndef DVM_MANAGED_WORKERS_H
#define DVM_MANAGED_WORKERS_H
#include "diagnostics/mailbox.h"


/* EN: Owner-published lifecycle intervals, not OS CPU states or heap permissions.
 * RU: Опубликованные владельцем интервалы жизни, не CPU states либо heap permissions. */
enum
{
    DVM_RESIDENT_STARTING,  /* Native startup/attach has not entered dispatch. */
    DVM_RESIDENT_DISPATCH,  /* Inside turn callback, including its native calls. */
    DVM_RESIDENT_SCHEDULER, /* Between turns, including admission/reaping. */
    DVM_RESIDENT_WAIT_WORK, /* Dedicated readiness wait, possibly spuriously awake. */
    DVM_RESIDENT_TEARDOWN,  /* Dispatch ended; owner/TLS cleanup in progress. */
    DVM_RESIDENT_DONE       /* TLS cleanup finished; registry slot may await join. */
};


/* EN: Diagnostic activity codes, independent from retained ProgState numbering.
 * RU: Диагностические activity codes, независимые от нумерации ProgState. */
enum
{
    DVM_ACTIVITY_UNKNOWN, /* No recognized publication. */
    DVM_ACTIVITY_READY,   /* Ready state; not a guarantee of CPU execution. */
    DVM_ACTIVITY_SEND,    /* Channel send registration. */
    DVM_ACTIVITY_RECEIVE, /* Channel receive registration. */
    DVM_ACTIVITY_ALT,     /* Channel alternative registration. */
    DVM_ACTIVITY_NATIVE,  /* Native continuation released admission. */
    DVM_ACTIVITY_TABLE,   /* Cooperative table admission wait. */
    DVM_ACTIVITY_CHILD,   /* Child retirement/cancellation reply wait. */
    DVM_ACTIVITY_DEBUG,   /* Retained debug state, no new pause feature. */
    DVM_ACTIVITY_EXITING, /* Published process exit. */
    DVM_ACTIVITY_BROKEN   /* Published process failure. */
};


/* EN/RU: Copied native metadata; lifetime ID is not a pointer or managed authority.
 * Копия native metadata; lifetime ID не pointer и не managed authority. */
typedef struct DVM_ResidentCard
{
    unsigned long id;         /* Stable lifetime identity within this service. */
    int pid;                  /* Permanent PID copied before resident execution. */
    int done;                 /* TLS destruction completed, published under metadata mutex. */
    int phase;                /* Atomic owner publication; no ordering of managed data is implied. */
    int activity;             /* Last published execution state; independent from phase. */
    unsigned long dispatches; /* Saturating callback entries, not instruction count. */
    unsigned long turns;      /* Saturating completed guest callbacks; read before dispatches. */
    unsigned long waits;      /* Saturating readiness condition-wait entries, including spurious wakes. */
} DVM_ResidentCard;


/* EN/RU: Copy ascending IDs after cursor into 1..256 disjoint native cards.
 * Caller guarantees live service through return; no concurrent service stop.
 * count/more are copied outputs; refusal leaves outputs unchanged. This is one
 * page, not a coherent multi-page snapshot; retired/reaped IDs may disappear.
 * Копировать ascending IDs после cursor в 1..256 disjoint native cards.
 * Caller удерживает service до return без concurrent service stop. count/more —
 * copied outputs; отказ не меняет output. Pages не образуют общий atomic snapshot. */
int dvm_resident_observe(
    unsigned long after, DVM_ResidentCard *cards, unsigned int capacity, unsigned int *count, int *more);


/* EN: One coherent registry partition under its metadata guard; no heap data.
 * RU: Согласованное разделение registry под metadata guard; без heap data. */
typedef struct DVM_ResidentSummary
{
    unsigned long created;    /* Installed native slots since service start. */
    unsigned long registered; /* Slots still linked, including completed ones. */
    unsigned long reaping;    /* Detached slots through join and deferred accounting. */
    unsigned long reaped;     /* Joined slots accounted by rootless reapers. */
    unsigned long done;       /* Completed slots within registered, not an additional partition. */
} DVM_ResidentSummary;


/* EN: Copy a coherent summary to disjoint native output. Caller keeps service
 * alive with no concurrent stop completion. NULL/unavailable refuses unchanged.
 * RU: Копировать согласованную summary в disjoint native output. Caller удерживает
 * service до возврата без concurrent stop completion. NULL/unavailable — без изменений. */
int dvm_resident_summary(DVM_ResidentSummary *summary);

/* EN: Caller owns original requester PID; exact ID is observational, not control
 * authority. Service/slot lifetime is held only through synchronous native copy.
 * RU: Caller владеет original requester PID; exact ID не control authority.
 * Service/slot lifetime удерживается только до возврата synchronous native copy. */
int dvm_resident_debug_request(unsigned long id, int requester, int64_t cookie, unsigned long *ticket);
int dvm_resident_debug_reply(unsigned long id, int requester, unsigned long ticket, DVM_DiagnosticSample *sample);
int dvm_resident_debug_forget(unsigned long id, int requester, unsigned long ticket);


enum
{
    DVM_MANAGED_TURN_IDLE,
    DVM_MANAGED_TURN_GUEST,
    DVM_MANAGED_TURN_MAINTENANCE
};


/* EN: Start the resident registry on the active controller.
 * turn and cancel_waits are borrowed callbacks until stop. diagnostics enables
 * lifecycle output. Return zero on invalid state or native allocation failure.
 * RU: Запустить реестр закреплённых потоков на активном контроллере.
 * Обратные вызовы заимствованы до stop. При ошибке вернуть ноль. */
int dvm_resident_threads_start(int diagnostics, int (*turn)(void), void (*cancel_waits)(void));


/* EN: Start and retire one resident on its permanently assigned thread.
 * RU: Запуск и завершение процесса на закреплённом потоке. */
struct Prog;
int dvm_managed_workers_start_guest(struct Prog *guest);
int dvm_runtime_prepare_resident(struct Prog *guest);
struct Prog *dvm_managed_workers_guest(void);
void dvm_managed_workers_guest_retired(int pid);


/* EN: Query a staged native reply after inspecting the resident's FIFO.
 * RU: Проверить извлечённый ответ после проверки FIFO процесса. */
int dvm_managed_workers_reply_in_transit(struct Prog *guest);


/* EN: Take the current owner's staged reply, if any.
 * RU: Забрать извлечённый ответ текущего владельца, если он есть. */
struct DVM_OwnerMessage *dvm_managed_workers_take_reply(struct Prog *guest);
void dvm_managed_workers_finish_reply(struct Prog *guest);


/* Signal runnable work or a pending native request. The caller must own
 * NativeWorld or otherwise guarantee service lifetime through the signal. */
void dvm_managed_workers_wake(void);
void dvm_managed_workers_wake_guest(struct Prog *guest);


/* Yield an empty controller turn through FIFO heap admission. May wait for
 * another executor or collector. Return zero for a worker/protocol failure.
 * idle requests a bounded native sleep OUTSIDE the heap before reentry.
 * No current Prog, native token or C-held managed root may be live here. */
int dvm_managed_workers_yield(int idle);


/* Park a rootless non-UI controller awaiting logical channel work. Heap is
 * released; pending checks native requests after the wake epoch is captured.
 * The callback is invoked without the scheduler mutex. */
int dvm_managed_workers_park(int (*pending)(void));


/* Count one controller scheduler turn, only when the optional service is live. */
void dvm_managed_workers_record(void);


/* Count one ordinary file operation on the ACTIVE heap owner. No handle/root is
 * retained; the separate per-handle FIFO serializes native cursor access. */
void dvm_managed_workers_file_call(void);


/* Authorize only the live helper's ACTIVE claimed turn for copied UI RPC.
 * Never authorizes UI main, foreign TLS or a native waiter; no root is returned.
 * Requires the caller's current phase check before reading shared heap identity. */
int dvm_managed_workers_current(void);


/* EN: Stop and join helpers, leaving the controller idle for collector shutdown.
 * RU: Остановить и присоединить исполнителей, оставив контроллер в IDLE.
 * diagnostics reports joined counters. No-op if the service was never started. */
int dvm_managed_workers_stop(int diagnostics);
#endif
