/* Copyright (c) 2026 bhgv
 * EN: Private collector lane storage; all shared fields use sync.
 * RU: Закрытое состояние потоков сборки; общие поля защищены sync.
 */
#ifndef DVM_GC_WORKER_PRIVATE_H
#define DVM_GC_WORKER_PRIVATE_H
#include "disvm/gc_workers.h"
#include "thread_port.h"
#include "channel_sync.h"


/* EN: Coalesced native queue notifications. RU: Объединяемые уведомления очередей. */
enum
{
    GC_PASS_REQUEST = 1,     /* Full lane pass requested. */
    GC_CANDIDATE_REQUEST = 2 /* Candidate discovery requested. */
};


/* [GC:] EN: Native collector lane, owned until its OS thread joins.
 * RU: Нативный поток сборки, принадлежащий службе до завершения join. */
typedef struct DVM_GCWorker
{
    struct DVM_GCWorkers *owner; /* Borrowed service. */
    DVM_GCKind kind;             /* Stable lane identity. */
    FVM_PortThread *thread;      /* Owned native handle. */
    DVM_NativeContext *context;  /* Worker-owned bound collector context. */
    size_t passes;               /* Completed callbacks under service mutex. */
    int pending;                 /* Coalesced request bits. */
} DVM_GCWorker;


/* [GC:] EN: Native service state. Backend owns and roots deferred objects;
 * requests carry only bits. sync protects lane state, never callback bodies.
 * RU: Нативное состояние службы. Backend владеет отложенными объектами и их
 * корнями; запросы содержат лишь биты. sync защищает состояние, не обработчики. */
struct DVM_GCWorkers
{
    FVM_PortSync *sync;         /* Request/lease/state mutex and condition. */
    DVM_GCWorkersConfig config; /* Copied callbacks; user remains borrowed. */
    DVM_GCWorker Fast;          /* Named RC worker group. */
    DVM_GCWorker Duke;          /* Optional shared large-graph worker. */
    DVM_GCWorker Cycles;        /* Named cycle worker group. */
    unsigned int started;       /* Context startup handshakes. */
    DVM_GCKind turn;            /* Round-robin start among pending lanes. */
    unsigned int active;        /* Bit mask of callbacks in flight. */
    unsigned int exclusive;     /* Lane bit owning the retained-collector gate. */
    unsigned int shared_peak;   /* Maximum simultaneous callbacks in parallel mode. */
    int stopping;               /* Cooperative shutdown. */
    int failed;                 /* Latched failure; never silently retried as success. */
};


/* [GC:] EN: Run worker's requested pass; executed reports callback execution.
 * The bound worker owns no service mutex. Return DONE/MORE/FAILED; concurrent
 * callbacks own their barriers and roots and never acquire a world stop here.
 * RU: Выполнить requested проход worker; executed сообщает о вызове обработчика.
 * Мьютекс службы освобождён. Вернуть DONE/MORE/FAILED; конкурентные обработчики
 * сами защищают объекты и корни, общая остановка здесь не захватывается.
 */
DVM_GCWorkResult dvm_gc_worker_collect(DVM_GCWorker *worker, int requested, int *executed);
#endif
