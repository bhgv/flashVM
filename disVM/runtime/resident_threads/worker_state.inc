/* Copyright (c) 2026 bhgv
 * EN: Private native slots and metadata ownership.
 * RU: Закрытые нативные слоты и владение метаданными.
 */


/* EN: One resident owns live execution fields until done; readers use join.
 * Native identity and registry links use service.sync, wake state uses wake.
 * RU: Процесс владеет полями исполнения до done; чтение счётчиков требует join.
 * Идентичность и связи реестра защищает service.sync, ожидания — wake.
 */
typedef struct ManagedWorker
{
    struct ManagedWorker *wake_next;   /* Registered under service.sync until thread exit. */
    DVM_OwnerMessage *staged_reply;    /* Owner-only reply removed before heap admission. */
    int reply_in_transit;              /* Published to the controller until the owner applies the reply. */
    DVM_DiagnosticLocation location; /* Owner-only copied last execution boundary. */
    unsigned long location_sequence; /* Owner-only saturated publication sequence. */
    DVM_DiagnosticMailbox *debug;    /* Independent bounded native query/reply credits. */
    FVM_PortSync *wake;                /* Dedicated readiness condition and generation lock. */
    int startup_committed;             /* Creator has installed the slot, under wake. */
    int wake_stopped;                  /* Stop/failure latch under wake. */
    struct ManagedWorker *next;        /* Native registry link protected by service.sync. */
    unsigned long id;                  /* Stable lifetime ID; never reused while the service exists. */
    Prog *guest;                       /* Borrowed until retirement on its own thread. */
    int guest_pid;                     /* Permanent resident PID; initialized before execution. */
    int guest_retired;                 /* Set by this guest thread after managed retirement. */
    unsigned long observed_dispatches; /* Atomic saturated callback entries; single owner writer. */
    unsigned long observed_turns;      /* Atomic saturated completed guest turns; owner writer. */
    unsigned long observed_waits;      /* Atomic saturated readiness waits; owner writer. */
    int observation_state;             /* Native atomic copy; remains readable after Prog destruction. */
    int observation_phase;             /* Atomic owner-published interval; no managed data ordering. */
    int done;                          /* Published under service.sync after TLS destruction. */
    DVM_ThreadSlot *thread;            /* Registrar handle, joined before this slot is reset. */
    uintptr identity;                  /* Context identity under service.sync; zero before context destruction. */
    unsigned long turns;               /* Successful bytecode turns executed by this helper. */
    unsigned long handoffs;            /* Completed rootless scheduler releases, distinct from native I/O. */
    DVM_HeapOwnerStats owner;          /* Copied counters before the helper detaches. */
} ManagedWorker;


/* EN: Service owns slots through join. sync protects shared lifecycle/counters;
 * never hold it while acquiring heap admission. Launch callbacks stay immutable.
 * RU: Сервис владеет слотами до join. sync защищает жизненный цикл и счётчики;
 * не удерживать его при входе в кучу. Стартовые callbacks неизменяемы.
 */
typedef struct ManagedWorkers
{
    DVM_ThreadRegistrar *registrar;  /* Owns OS handles, independent of managed admission. */
    FVM_PortSync *sync;              /* Protect stop/failure/ready publication. */
    DVM_NativeWorld *world;          /* Borrowed shared heap domain, immutable through join. */
    int (*turn)(void);               /* Immutable resident dispatch callback; executions may overlap. */
    int stopping;                    /* Cooperative stop request, under sync. */
    int failed;                      /* Latched attach/dispatch/retirement/start error. */
    int diagnostics;                 /* Emit lifecycle evidence only for --stats. */
    unsigned long live;              /* Live helpers, under service.sync. */
    unsigned long peak;              /* Peak live executors, under service.sync. */
    unsigned long created;           /* Successfully created helpers, under service.sync. */
    unsigned long next_id;           /* Lifetime IDs allocated by the startup owner. */
    unsigned long generation;        /* Runnable-work publication under sync; prevents lost wakeups. */
    unsigned long reaping;           /* Detached reaper-owned slots, under service.sync. */
    unsigned long reaped;            /* Joined helpers, under service.sync. */
    unsigned long retired_turns;     /* Joined turns, under service.sync. */
    unsigned long retired_releases;  /* Joined releases, under service.sync. */
    unsigned long retired_reentries; /* Joined reentries, under service.sync. */
    void (*cancel_waits)(void);      /* Native wakeup hook for a fatal worker failure. */
    unsigned long controller;        /* Successful controller turns. */
    unsigned long files;             /* Descriptor-call count under service.sync; no VM execution lock. */
    unsigned long ui_requests;       /* Helper UI role admissions, under service.sync. */
    ManagedWorker *wake_slots;       /* Dedicated conditions protected by service.sync. */
    ManagedWorker *helpers;          /* Individually allocated stable slots; never realloc live storage. */
} ManagedWorkers;

static __thread ManagedWorker *current_guest; /* Dedicated slot bound through native TLS teardown. */
static ManagedWorkers service;                /* One process-wide runtime; lifetime ends after join. */


/* EN: Publish phase for this owner's borrowed live slot; no allocation, lock or
 * managed access. Observers gain no heap ordering/authority. No result or failure.
 * RU: Опубликовать phase своего живого слота worker; без allocation, lock либо
 * managed access. Observer не получает heap ordering/authority. Нет результата. */
static void publish_resident_phase(ManagedWorker *worker, int phase)
{
    __atomic_store_n(&worker->observation_phase, phase, __ATOMIC_RELAXED);
}


/* EN/RU: Service only this permanent owner's bounded diagnostic mailbox.
 * Обслужить только bounded diagnostic mailbox своего permanent owner. */
static void service_debug_mailbox(ManagedWorker *worker);
