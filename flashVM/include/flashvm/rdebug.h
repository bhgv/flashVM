/* Owner-thread debugger resource: copied snapshots, independent observers and one controller. */
#ifndef FLASHVM_RDEBUG_H
#define FLASHVM_RDEBUG_H /* Guard the additive C89 debugger resource ABI. */
#include "debug.h"
#include "revents.h"
#define FVM_RDEBUG_VERSION 1U   /* Version of snapshot/frame payloads below. */
#define FVM_RDEBUG_NAME 127U    /* Maximum copied function-name bytes, excluding NUL. */
#define FVM_RDEBUG_SOURCE 255U  /* Maximum copied source-name bytes, excluding NUL. */
#define FVM_RDEBUG_FRAMES 16U   /* Maximum cached innermost frames from each stop. */
#define FVM_RDEBUG_SNAPSHOT 1UL /* READ: empty request; return a copied snapshot payload. */
#define FVM_RDEBUG_FRAME 2UL    /* READ: expected-stop and frame-index as unsigned S integers. */
#define FVM_RDEBUG_BREAKPOINT                                                                      \
    3UL /* CONTROL: offset, enabled, name-size as S integers; function bytes. */
#define FVM_RDEBUG_CLAIM 4UL /* CONTROL: empty request; first claimant becomes controller. */
#define FVM_RDEBUG_RELEASE                                                                         \
    5UL                       /* CONTROL: empty request; current controller relinquishes control. */
#define FVM_RDEBUG_RESUME 6UL /* CONTROL: expected-stop S integer; current controller only. */
#define FVM_RDEBUG_STOP_EVENT 1UL /* Observer event carrying the new stopped snapshot. */
#define FVM_RDEBUG_RESUME_EVENT                                                                    \
    2UL /* Snapshot at callback release, not later execution progress. */
#define FVM_RDEBUG_ABORT_EVENT                                                                     \
    3UL /* Host service or snapshot/publication failure aborted the stop. */
#define FVM_RDEBUG_RUNNING                                                                         \
    0U                        /* No active breakpoint callback; last stop data may remain cached.  \
                               */
#define FVM_RDEBUG_STOPPED 1U /* VM is inside the synchronous owner breakpoint callback. */
#define FVM_RDEBUG_ABORTED 2U /* The last owner stop returned an error to VM execution. */
#define FVM_RDEBUG_FUNCTION_TRUNCATED 1U /* Function label exceeded the copied capacity. */
#define FVM_RDEBUG_SOURCE_TRUNCATED 2U   /* Source label exceeded the copied capacity. */
#define FVM_RDEBUG_FRAMES_TRUNCATED 4U   /* Live stack had more than the cached frame capacity. */
#define FVM_RDEBUG_FLAGS 7U              /* Defined snapshot truncation bits. */
#define FVM_RDEBUG_SNAPSHOT_FIELDS 10U   /* Canonical integer fields preceding copied labels. */
#define FVM_RDEBUG_FRAME_FIELDS 8U       /* Canonical integer fields preceding the frame label. */
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct FVM_RDebug
        FVM_RDebug; /* Opaque native adapter borrowing context and dispatcher. */
    /* Copied stop metadata; no managed references, bytecode pointers or machine-code addresses. */
    typedef struct FVM_RDebugSnapshot
    {
        unsigned int state;   /* RUNNING, STOPPED or ABORTED; not a general VM execution state. */
        unsigned int flags;   /* Explicit FUNCTION/SOURCE/FRAMES truncation bits. */
        unsigned long stop;   /* Nonzero monotonically increasing stop incarnation; never wraps. */
        unsigned long offset; /* AVM1 action offset reported by the breakpoint callback. */
        unsigned long line;   /* Source line, or zero when unmapped. */
        unsigned long frames; /* Number of cached frames, at most FVM_RDEBUG_FRAMES. */
        unsigned long controller; /* Current claimed session, or zero; not an authority token. */
        char function[FVM_RDEBUG_NAME + 1]; /* Copied, possibly truncated function label. */
        char source[FVM_RDEBUG_SOURCE + 1]; /* Copied, possibly truncated source label. */
    } FVM_RDebugSnapshot;
    /* Cached structural frame metadata; values/locals/arguments remain outside this adapter. */
    typedef struct FVM_RDebugFrame
    {
        unsigned long offset;    /* Action offset captured at the stop. */
        unsigned long argc;      /* Supplied argument count, not argument values. */
        unsigned long registers; /* Materialized register-slot count, not register values. */
        unsigned int engine;     /* FVM_CRASH_ENGINE_INTERPRETER/JIT/NATIVE. */
        unsigned int forced;     /* Whether entry arguments were explicitly preserved. */
        unsigned int truncated;  /* Function label was longer than FVM_RDEBUG_NAME. */
        char function[FVM_RDEBUG_NAME + 1]; /* Native copy valid independently of VM collection. */
    } FVM_RDebugFrame;
    /* Called only by the stopped VM owner after pumping available commands. It must service/wait
     * for native transport work without reentering/collecting/destroying this context. Return OK to
     * pump again; any other VM result aborts this execution. A blocking wait must handle shutdown
     * and lost wakeups itself. There is no implicit thread, busy polling, timeout or socket. */
    typedef FVM_Result (*FVM_RDebugService)(FVM_RDebug *, void *userdata);
    /* Owner allocates/registers the native resource, initially unclaimed. Both outputs change only
     * on OK. Explicitly install fvm_rdebug_breakpoint with fvm_debug_set_breakpoint_handler
     * afterwards; create does not overwrite an existing debugger hook. Context, dispatcher and
     * service userdata must outlive the adapter. No VM ownership/GC root is transferred. */
    FVM_API FVM_RResult fvm_rdebug_create(FVM_Context *, FVM_RDispatch *, const char *path,
                                          FVM_RDebugService, void *userdata, FVM_RDebug **,
                                          FVM_RHandle *resource);
    /* Owner revokes this resource's rights for one session and clears its controller claim.
     * Also call dispatcher_disconnect after quiescing its transport to remove other resource state.
     * Controller loss does not silently resume a stopped VM; the host service decides recovery. */
    FVM_API FVM_RResult fvm_rdebug_revoke(FVM_RDebug *, unsigned long session);
    /* Install as the ordinary synchronous breakpoint callback, userdata=adapter. Captures native
     * metadata, publishes STOP and services queued commands until the current controller resumes
     * this exact stop, or host service fails. Never call manually or from a transport worker. */
    FVM_API FVM_Result fvm_rdebug_breakpoint(FVM_Context *, const FVM_DebugBreakpoint *,
                                             void *adapter);
    /* Owner uninstalls the VM hook first and quiesces transport users. BUSY during a stop/handler;
     * unregister invalidates watches and queued old-generation work. Free only on successful
     * detach. */
    FVM_API FVM_RResult fvm_rdebug_destroy(FVM_RDebug **);
    /* Client-side strict decoder for SNAPSHOT replies and all three event kinds. Exact extent,
     * version, flags, counts and counted strings are checked; output unchanged on error. */
    FVM_API FVM_RResult fvm_rdebug_snapshot_read(const void *, size_t, FVM_RDebugSnapshot *);
    /* Client-side decoder for a FRAME reply. This is copied metadata, not a live frame view. */
    FVM_API FVM_RResult fvm_rdebug_frame_read(const void *, size_t, FVM_RDebugFrame *);
#ifdef __cplusplus
}
#endif
#endif
