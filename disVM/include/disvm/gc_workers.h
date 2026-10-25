/* Two native GC workers with bounded passes and explicit stopped-world ownership. */
#ifndef DISVM_GC_WORKERS_H
#define DISVM_GC_WORKERS_H
#include "native_world.h"

#ifdef __cplusplus
extern "C"
{
#endif
/* Bound optional pacing so cooperative shutdown never waits on an arbitrary sleep. */
#define DVM_GC_MAX_PAUSE_US 1000000UL


/* One worker drains deferred RC work; the other advances cycle collection. */
typedef enum DVM_GCKind
{
    DVM_GC_FAST = 0,  /* Deferred reference-count work. */
    DVM_GC_CYCLES = 1 /* Incremental cyclic-garbage collection. */
} DVM_GCKind;


/* A bounded callback may request another pass, or report an unrecoverable failure. */
typedef enum DVM_GCWorkResult
{
    DVM_GC_FAILED = -1, /* Latch failure and reject subsequent work. */
    DVM_GC_DONE = 0,    /* This lane is finished until another request. */
    DVM_GC_MORE = 1     /* Reschedule another bounded pass. */
} DVM_GCWorkResult;
typedef struct DVM_GCWorkers DVM_GCWorkers;


/* Called only by its native collector while the world is stopped. The adapter
 * performs at most budget work units and must return normally on every path. */
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


/* [GC:] user/world/backend roots must outlive both joins. Pending RC objects must
 * remain rooted in the backend until processed; this API queues requests, not pointers. */
typedef struct DVM_GCWorkersConfig
{
    DVM_NativeWorld *world;             /* Borrowed coordination domain. */
    DVM_GCContextCreate create_context; /* Owned context factory for each collector. */
    DVM_WorldRoots roots;               /* Publish participant roots into backend root sets. */
    DVM_GCPass Fast;                    /* Deferred reference-count releases. */
    DVM_GCPass Cycles;                  /* Incremental reachability/cycle passes. */
    void *user;                         /* Borrowed backend and its synchronized queues. */
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
    int active;   /* 0 idle, 1 fast, 2 cycles; only one heap pass runs at a time. */
    int stopping; /* Shutdown requested; queued requests are discarded. */
    int failed;   /* Startup/callback/world protocol failure latched. */
} DVM_GCWorkersState;


/* Start two full OS threads and wait until both collector contexts are bound. */
DVM_GCWorkers *dvm_gc_workers_create(const DVM_GCWorkersConfig *config);


/* Coalesce a request; returns 0 on invalid kind, failure or shutdown. */
int dvm_gc_workers_request(DVM_GCWorkers *workers, DVM_GCKind kind);


/* Wait for both lanes to become idle. Do not call from an active mutator or
 * GC callback: the collector may be waiting for that same mutator to poll. */
int dvm_gc_workers_wait_idle(DVM_GCWorkers *workers);


/* Take a mutex-protected snapshot without waiting for collection completion. */
int dvm_gc_workers_state(DVM_GCWorkers *workers, DVM_GCWorkersState *out);


/* Cancel queued requests, finish the current pass, join both OS threads and free.
 * Backend keeps ownership of unprocessed work. No forced thread termination.
 * Fails without destroying if called from either collector thread. */
int dvm_gc_workers_destroy(DVM_GCWorkers *workers);
#ifdef __cplusplus
}
#endif
#endif
