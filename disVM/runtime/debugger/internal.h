/* Copyright (c) 2026 bhgv
 * EN: Private native debug registry and bounded request/result storage.
 * RU: Private native debug registry и bounded request/result storage.
 */
#ifndef DVM_DEBUG_INTERNAL_H
#define DVM_DEBUG_INTERNAL_H
#include "debugger.h"
#include "channel_sync.h"
#include "diagnostics/location.h"
#include "jit_fault/signals.h"
#include <pthread.h>
#include <sys/types.h>


/* EN/RU: Stop reasons distinguish an instruction boundary from an audited native call.
 * Причины stop различают instruction boundary и audited native call. */
enum
{
    DebugRunning = 0, /* Last publication only, not frozen memory. */
    DebugPaused = 1,  /* Requested boundary/native continuation is retained. */
    DebugFaulted = 2  /* Original managed throw has not reached handler/unwind. */
};


/* [GC:] EN: No owned managed reference. Frozen publication is borrowable ONLY
 * while wake is held AND native_ready is set; gate blocks source reentry.
 * RU: Нет owned managed references. Frozen publication заимствуется ТОЛЬКО под
 * wake при native_ready; gate запрещает source reentry. */
typedef struct DebugTarget
{
    struct DebugTarget *next;        /* Registry owns native card until owner withdraws. */
    FVM_PortSync *wake;              /* Protect every field except immutable pid/next. */
    unsigned long steps;             /* Saturating admitted-op count, debug mode only. */
    int pid;                         /* VM-lifetime PID, not a cross-VM capability. */
    int requested;                   /* Cooperative pause request, never pthread suspension. */
    int terminate;                   /* Source owner applies managed termination. */
    int closing;                     /* Runtime shutdown releases every native gate. */
    int native_ready;                /* Matching source NATIVE publication is stable. */
    int state;                       /* DebugRunning/Paused/Faulted. */
    int boundary;                    /* Current owner is before an instruction, permitting single-step. */
    int step_after;                  /* Pause on the next original Dis instruction boundary. */
    int refused;                     /* Original fault could not be parked safely. */
    int fault;                       /* Original throw still precedes exception handler. */
    Prog *process;                   /* Borrowed only while native_ready and wake held. */
    DVM_JitFault native_fault;       /* Copied CPU signal context, never used to resume execution. */
    REG registers;                   /* Diagnostic publication, never resume source. */
    Inst instruction;                /* Exact last admitted Dis opcode/operands. */
    DVM_DiagnosticLocation location; /* Copied immutable module display/PC. */
    char error[128];                 /* Bounded copied original error; not a heap string. */
} DebugTarget;


/* EN/RU: Process-wide supervisor lifetime; registry lock precedes target wake.
 * Lifetime supervisor процесса; registry lock предшествует target wake. */
typedef struct DebugService
{
    FVM_PortSync *lock;        /* Protect native target links through destruction. */
    DebugTarget *targets;      /* Live VM targets, excluding GC/I/O/debug helper. */
    pthread_t thread;          /* Independent native-only control transport thread. */
    int input;                 /* Owned pipe input, or -1 for socket transport. */
    int output;                /* Owned pipe output, separate from guest stdout. */
    int listener;              /* Owned local socket; no ordinary guest FD use. */
    int stopping;              /* Atomic stop hint for bounded transport poll. */
    unsigned long made;        /* Native cards allocated, atomic debug-only accounting. */
    unsigned long freed;       /* Native cards withdrawn/destroyed, atomic. */
    unsigned long unhandled;   /* Continued unhandled errors affect final application status. */
    unsigned long faults;      /* Managed first-chance fault events, atomic. */
    unsigned long inspections; /* Read-only frozen inspection attempts, atomic. */
    int started;               /* pthread lifetime has begun. */
    dev_t device;              /* Socket pathname identity before unlink. */
    ino_t inode;               /* Refuse cleanup of a replacement pathname. */
    char session[33];          /* Random VM session identity prevents stale PID commands on reattach. */
    char path[108];            /* Owned bounded sockaddr_un pathname. */
} DebugService;

extern DebugService dvm_debug_service; /* Initialized before VM owners; freed after join. */


/* EN/RU: Serve bounded one-line native commands; no guest callback or arbitrary pointers.
 * Обслужить bounded native command; без guest callback либо arbitrary pointers. */
void dvm_debug_command(const char *line, char *output, size_t capacity);


/* EN/RU: Generate one native session identity before accepting control commands.
 * Создать native session identity до приёма control commands. */
int dvm_debug_session_init(void);


/* [GC:] EN/RU: Inspector lazily attaches its own GC owner; detach before GC shutdown.
 * Inspector лениво подключает собственного GC owner; отключить до GC shutdown. */
int dvm_debug_inspector_enter(void);
void dvm_debug_inspector_leave(void);
void dvm_debug_inspector_stop(void);


/* [GC:] EN: Inspect frozen target under wake/native gate, using this helper's ACTIVE
 * admission and ordinary shared range guards. Output is native bounded text only.
 * RU: Исследовать frozen target под wake/native gate, с ACTIVE admission helper
 * и обычными shared range guards. Output — только bounded native text. */
void dvm_debug_inspect(DebugTarget *target, const char *command, char *output, size_t capacity);


/* EN/RU: Run local transport; never execute a Prog or become its register owner.
 * Обслуживать local transport; не исполнять Prog и не становиться owner его REG. */
void *dvm_debug_transport(void *argument);


/* EN/RU: Serve explicit inherited pipes; debugger disconnect never resumes guests.
 * Обслуживать explicit inherited pipes; disconnect не возобновляет guests. */
void dvm_debug_pipe_loop(void);
#endif
