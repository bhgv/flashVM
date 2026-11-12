/* Copyright (c) 2026 bhgv
 * EN: Native collector workers with concurrent or retained bounded callbacks.
 * RU: Системные потоки сборки с конкурентными или резервными проходами.
 */
#ifndef DISVM_GC_WORKERS_H
#define DISVM_GC_WORKERS_H
#include "native_world.h"

#ifdef __cplusplus
extern "C"
{
#endif
/* Bound optional pacing so cooperative shutdown never waits on an arbitrary sleep. */
#define DVM_GC_MAX_PAUSE_US 1000000UL


/* Fast drains deferred RC; Cycles and optional Duke discover garbage. */
typedef enum DVM_GCKind
{
    DVM_GC_FAST = 0,   /* Deferred reference-count work. */
    DVM_GC_CYCLES = 1, /* Incremental cyclic-garbage collection. */
    DVM_GC_DUKE = 2    /* Optional shared large-graph discovery. */
} DVM_GCKind;


/* A bounded callback may request another pass, or report an unrecoverable failure. */
typedef enum DVM_GCWorkResult
{
    DVM_GC_FAILED = -1, /* Latch failure and reject subsequent work. */
    DVM_GC_DONE = 0,    /* This lane is finished until another request. */
    DVM_GC_MORE = 1     /* Reschedule another bounded pass. */
} DVM_GCWorkResult;
typedef struct DVM_GCWorkers DVM_GCWorkers;


/* [GC:] EN: Called on its bound collector context. A retained callback owns the
 * stopped world; a concurrent callback owns no world stop and must protect all
 * accessed objects itself. budget bounds backend work; user is borrowed through
 * worker joins. Return DONE, MORE for retry (including contention), or FAILED.
 * Never escape by longjmp or wait for this service to become idle.
 * RU: Вызывается на привязанном контексте сборщика. Резервный обработчик владеет
 * остановленным миром; конкурентный сам защищает доступ к объектам. budget
 * ограничивает работу, user заимствован до join. Вернуть DONE, MORE для повтора
 * (включая конфликт) или FAILED. Запрещены longjmp и ожидание простоя службы. */
typedef DVM_GCWorkResult (*DVM_GCWork)(DVM_NativeContext *, size_t budget, void *user);
/* Create an owned collector context; core users supply native_process_create.
 * The service binds/unbinds/destroys it on its worker, never on a mutator. */
typedef DVM_NativeContext *(*DVM_GCContextCreate)(void *user);


/* Named pass configuration: budget units are defined by the backend, not bytes. */
typedef struct DVM_GCPass
{
    DVM_GCWork run;         /* Bounded retained RC/GC adapter, not a second collector algorithm. */
    size_t budget;          /* Positive maximum units for one stopped-world pass. */
    unsigned long pause_us; /* Optional MORE-pass pacing, outside world/heap locks. */
} DVM_GCPass;


/* [GC:] user/world/backend roots must outlive all worker joins. Pending RC objects must
 * remain rooted in the backend until processed; this API queues requests, not pointers. */
typedef struct DVM_GCWorkersConfig
{
    DVM_NativeWorld *world;             /* Borrowed coordination domain. */
    DVM_GCContextCreate create_context; /* Owned context factory for each collector. */
    DVM_WorldRoots roots;               /* Publish participant roots into backend root sets. */
    DVM_GCPass Fast;                    /* Deferred reference-count releases. */
    DVM_GCPass Cycles;                  /* Incremental reachability/cycle passes. */
    void *user;                         /* Borrowed backend and its synchronized queues. */
    int cycles_quiescent_only;          /* Cycles claims idle admission without parking guests. */
    int parallel_shared;                /* Shared callbacks on distinct lanes may overlap. */


    int fast_quiescent_only; /* Fast claims idle admission without parking guests. */


    unsigned int (*cycles_shared)(DVM_NativeContext *, size_t, void *); /* Optional candidate walk. */
    unsigned int (*fast_shared)(DVM_NativeContext *, size_t, void *);   /* Optional shared leaf work. */
    size_t duke_budget;                                                 /* Large-graph work units per shared pass. */


    unsigned long duke_pause_us;                                      /* Pacing outside heap admission. */
    unsigned int (*duke_shared)(DVM_NativeContext *, size_t, void *); /* Shared large graphs. */
    int (*fast_ready)(void *user); /* Optional native-only queue check before a Fast stop. */
    DVM_GCWork concurrent_fast;    /* Paired concurrent route: no retained fallback. */
    DVM_GCWork concurrent_cycles;  /* Required with concurrent_fast and parallel_shared. */
} DVM_GCWorkersConfig;


/* One lane's coherent observation, never a promise about future work. */
typedef struct DVM_GCLaneState
{
    size_t passes; /* Completed callbacks, excluding cancelled queued requests. */
    int pending;   /* At least one coalesced pass requested. */
} DVM_GCLaneState;


/* Mutators and collectors can inspect this without waiting for a pass to finish. */
typedef struct DVM_GCWorkersState
{
    DVM_GCLaneState Fast;
    DVM_GCLaneState Cycles;
    DVM_GCLaneState Duke;
    int active;               /* 0 idle, otherwise one active lane ID plus one. */
    unsigned int shared_peak; /* Peak simultaneous callbacks in parallel mode. */
    int stopping;             /* Shutdown requested; queued requests are discarded. */
    int failed;               /* Startup/callback/world protocol failure latched. */
} DVM_GCWorkersState;


/* [GC:] EN: Start bound Fast/Cycles and optional Duke contexts. config is copied;
 * world/user outlive joins. Set both concurrent callbacks and parallel_shared
 * for concurrent dispatch: retained run/roots/shared callbacks are bypassed for
 * Fast/Cycles, even on DONE/FAILED. Their run/roots pointers may be NULL. An
 * incomplete pair, zero budget or invalid pacing fails before any thread starts.
 * Returns owned workers or NULL; creation waits only for context startup.
 * RU: Запустить Fast/Cycles и необязательный Duke. config копируется, world/user
 * живут до join. Оба concurrent-обработчика вместе с parallel_shared выбирают
 * конкурентный путь: резервные run/roots/shared не вызываются даже при DONE/FAILED.
 * Их run/roots могут быть NULL. Неполная пара, нулевой бюджет и неверная задержка
 * отвергаются до запуска потоков. Вернуть службу или NULL; ждать только запуск.
 */
DVM_GCWorkers *dvm_gc_workers_create(const DVM_GCWorkersConfig *config);


/* Coalesce a request; returns 0 on invalid kind, failure or shutdown. */
int dvm_gc_workers_request(DVM_GCWorkers *workers, DVM_GCKind kind);


/* Request shared candidate work without requesting the legacy trace. */
int dvm_gc_workers_request_candidates(DVM_GCWorkers *workers);


/* Wait for all configured lanes to become idle. Do not call from an active mutator or
 * GC callback: the collector may be waiting for that same mutator to poll. */
int dvm_gc_workers_wait_idle(DVM_GCWorkers *workers);


/* [GC:] EN: Request and await one subsequent Fast attempt, not an empty queue.
 * Borrow workers through return; publish roots and leave heap admission first.
 * Return zero on failure/shutdown, overflow or collector self-wait.
 * RU: Запросить и дождаться следующей попытки Fast, а не пустой очереди.
 * workers заимствован; сначала опубликовать корни и выйти из допуска.
 * Ошибка, остановка, переполнение или самоожидание возвращают ноль. */
int dvm_gc_workers_step_fast(DVM_GCWorkers *workers);


/* Wait for the Fast lane only. Caller must hold no heap admission. */
int dvm_gc_workers_wait_fast_idle(DVM_GCWorkers *workers);


/* Take a mutex-protected snapshot without waiting for collection completion. */
int dvm_gc_workers_state(DVM_GCWorkers *workers, DVM_GCWorkersState *out);


/* Cancel queued requests, finish the current pass, join all OS threads and free.
 * Backend keeps ownership of unprocessed work. No forced thread termination.
 * Fails without destroying if called from a collector thread. */
int dvm_gc_workers_destroy(DVM_GCWorkers *workers);
#ifdef __cplusplus
}
#endif
#endif
