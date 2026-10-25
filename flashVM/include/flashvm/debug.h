/* FlashVM: Public embedding API declarations: debug.
 * File: include/flashvm/debug.h.
 */

#ifndef FLASHVM_DEBUG_H
/* Define FLASHVM DEBUG H constant or compilation switch. */
#define FLASHVM_DEBUG_H
#include "channel.h"
#include "crash.h"
#include "table.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Borrowed activation view valid only inside a breakpoint or fault debugger callback. */
typedef struct FVM_DebugFrame
{
    const char *function;           /* Counted function label; <root> for top-level actions. */
    size_t function_size;           /* Complete label byte length. */
    unsigned long offset;           /* Current/caller-suspended AVM1 action offset. */
    FVM_CrashEngine engine;         /* Actual interpreter/JIT/host-native body category. */
    int forced;                     /* Whether explicit entry argument preservation succeeded. */
    FVM_Result preservation_status; /* OK or failed FORCE entry-copy status. */
    unsigned int argc;              /* Number of supplied input arguments. */
    unsigned int register_count;    /* Number of existing materialized ABI register slots. */
    const void *bytecode;           /* Borrowed immutable original source body. */
    size_t bytecode_size;           /* Source body byte count. */
    const void *machine_code;       /* Borrowed active RX mapping, or null outside JIT. */
    size_t machine_code_size;       /* Active mapping payload byte count. */
} FVM_DebugFrame;


/* Count up to 256 live frames, innermost first. NOT_FOUND outside a debugger stop. */
FVM_API FVM_Result fvm_debug_frame_count(FVM_Context *, size_t *);


/* Borrow one live frame; pair with crash_argument/local/register queries during either stop. */
FVM_API FVM_Result fvm_debug_frame_at(FVM_Context *, size_t, FVM_DebugFrame *);


/* Borrowed location valid only during the synchronous interpreter breakpoint callback. */
typedef struct FVM_DebugBreakpoint
{
    const char *function; /* Function name; empty for top-level actions. */
    const char *source;   /* Mapped source name, possibly empty. */
    unsigned long offset; /* Action offset within the current body. */
    unsigned long line;   /* Mapped source line, or zero. */
} FVM_DebugBreakpoint;


/* Return OK to resume, another status to stop. Do not reenter, collect or destroy the VM.
     * Callbacks may set/remove breakpoints and change execution mode on the owning thread. */
typedef FVM_Result (*FVM_DebugBreakpointHandler)(FVM_Context *, const FVM_DebugBreakpoint *,
                                                 void *);


/* Set/remove an exact bytecode offset. Any breakpoint forces the named body's next entry
     * through the interpreter, including explicitly attached JIT bodies. Active native frames
     * finish unchanged; no in-place deoptimization. Empty function selects top-level blocks.
     * Function names identify all same-named bodies, not a unique function object. */
FVM_API FVM_Result fvm_debug_set_breakpoint(FVM_Context *, const char *function,
                                            unsigned long offset, int enabled);


/* Install a synchronous debugger host callback, or null to clear it; userdata is borrowed. */
FVM_API FVM_Result fvm_debug_set_breakpoint_handler(FVM_Context *, FVM_DebugBreakpointHandler,
                                                    void *);
/* Source mapping supplied by the embedding application; AVM1 updates offset.
       Unknown source/line is empty/zero. Copied snapshots contain no VM pointers. */


/* Update debug location. */
FVM_API FVM_Result fvm_debug_set_location(FVM_Context *, const char *, unsigned long);
/* Offsets are relative to the named function's AVM1 action block. Empty function
       names describe top-level bytecode. The nearest preceding entry maps an action. */


/* Implement debug map action for this subsystem. */
FVM_API FVM_Result fvm_debug_map_action(FVM_Context *, const char *function,
                                        unsigned long offset, const char *source,
                                        unsigned long line);


/* Define FVM_DebugMutex struct and its representation. */
typedef struct FVM_DebugMutex
{
    unsigned long identity /* Stable identity of this shared resource. */,
        origin /* Identifier of the resource origin. */,
        owner /* Owner responsible for the lifetime of this resource. */,
        waiter /* Identifier of the waiting participant. */,
        line /* Source line number associated with this entry. */,
        offset;              /* Byte offset identifying the instruction or data. */
    int state /* Locally observed ownership; remote authority owners may be unknown. */,
        deadlocked;          /* Whether the locally observed wait graph contains a cycle. */
    unsigned long owner_line /* Source line where the resource was acquired. */,
        owner_offset;        /* Bytecode offset where the resource was acquired. */
    char owner_source[256] /* Source name where the resource was acquired. */,
        owner_function[256]; /* Function that acquired the resource. */
    char source[256] /* Source name associated with this entry. */,
        function[256];       /* Function name associated with this source location. */
} FVM_DebugMutex;

/* A row per waiter, or one row with waiter=0 for an unawaited mutex.
       Exact coherent registry snapshot; insufficient capacity returns NOT_FOUND. */


/* Capture snapshot of debug mutex. */
FVM_API FVM_Result fvm_debug_mutex_snapshot(FVM_DebugMutex *, size_t, size_t *);


/* Endpoint-aware row: all fields are copied under the registry lock without network requests.
     */
typedef struct FVM_DebugMutexDetail
{
    FVM_DebugMutex state; /* Existing local ownership, waiter and source metadata. */
    int remote;           /* True for a remote proxy; foreign authority ownership is not inferred. */
    char address[512];    /* Copied capability URI, empty for a local mutex. */
} FVM_DebugMutexDetail;


/* Snapshot all local mutex/proxy rows with endpoints. Count is required; insufficient
       capacity returns NOT_FOUND without modifying rows. No distributed cycle claim is made. */
FVM_API FVM_Result fvm_debug_mutex_detail_snapshot(FVM_DebugMutexDetail *, size_t, size_t *);


/* Define FVM_DebugChannel struct and its representation. */
typedef struct FVM_DebugChannel
{
    unsigned long identity; /* Stable identity of this shared resource. */
    size_t
        pending /* Queued messages or complete unread file records; zero after file closure. */,
        capacity; /* Allocated storage limit; zero may denote an unbounded queue. */
    int closed /* Whether further operations have been closed. */,
        frozen;   /* Whether debugger interception has frozen delivery. */
} FVM_DebugChannel;

/* Freeze queue delivery or local file reads; producers/writers remain active. Closed frozen
       queues require explicit resume to drain. Compatible brokers control remote authority
       freezing; remote closure still rejects receives even if the authority queue has buffered
       records. */


/* Implement debug channel freeze for this subsystem. */
FVM_API FVM_Result fvm_debug_channel_freeze(FVM_Channel *, int);


/* Inspect local or compatible remote queue/file state without consuming records.
       Remote identity denotes the local proxy; remote counters are bounded to Uint32.
       Transport/read/framing failures leave output unchanged; older brokers reject new controls. */
FVM_API FVM_Result fvm_debug_channel_state(FVM_Channel *, FVM_DebugChannel *);
/* [GC:] Decode an independent snapshot without dequeueing, including compatible remote brokers.
       Resource capabilities remain live shared handles; their native state is not frozen.
       File indexes count unread records, with a 65536-record / 64 MiB inspection work limit;
       every outcome restores the cursor. Remote indexes are Uint32. Root returned objects
       before GC; decoding or transport failure leaves the result undefined. */


/* Implement debug channel message for this subsystem. */
FVM_API FVM_Result fvm_debug_channel_message(FVM_Context *, FVM_Channel *, size_t, FVM_Value *);


/* Define FVM_DebugChannelWait struct and its representation. */
typedef struct FVM_DebugChannelWait
{
    unsigned long context /* Owning interpreter context. */,
        channel /* Shared channel handle; ownership follows the enclosing API. */,
        line /* Source line number associated with this entry. */,
        offset;        /* Byte offset identifying the instruction or data. */
    int sending;       /* Whether the intercepted operation sends rather than receives. */
    char source[256] /* Source name associated with this entry. */,
        function[256]; /* Function name associated with this source location. */
} FVM_DebugChannelWait;

/* Active ActionScript send/receive waits, not inferred channel deadlocks. */


/* Wait for debug channel snapshot. */
FVM_API FVM_Result fvm_debug_channel_wait_snapshot(FVM_DebugChannelWait *, size_t, size_t *);


/* Bounded oldest-to-newest error intercept queue. Events copy locations and
       addresses. Overflow drops oldest events; clear only after successful copy. */
/* Define FVM_DebugResourceError struct and its representation. */
typedef struct FVM_DebugResourceError
{
    unsigned long context /* Owning interpreter context. */,
        resource /* Opaque shared or platform resource handle. */,
        line /* Source line number associated with this entry. */,
        offset;        /* Byte offset identifying the instruction or data. */
    FVM_Result result; /* Result produced by the operation. */
    char operation[32] /* Resource operation being performed or diagnosed. */,
        address[256] /* Network address identifying the endpoint. */,
        source[256] /* Source name associated with this entry. */,
        function[256]; /* Function name associated with this source location. */
} FVM_DebugResourceError;


/* Implement debug resource errors for this subsystem. */
FVM_API FVM_Result fvm_debug_resource_errors(FVM_DebugResourceError *, size_t, size_t *,
                                             int clear);
/* Snapshot-style inspection: borrowed handles; only call on the VM thread. */


/* Count debug object. */
FVM_API FVM_Result fvm_debug_object_count(FVM_Context *, size_t *);


/* Implement debug object at for this subsystem. */
FVM_API FVM_Result fvm_debug_object_at(FVM_Context *, size_t, FVM_Object **);


/* Count debug native function. */
FVM_API FVM_Result fvm_debug_native_function_count(FVM_Context *, size_t *);


/* Implement debug native function at for this subsystem. */
FVM_API FVM_Result fvm_debug_native_function_at(FVM_Context *, size_t, const char **);


/* Count debug script function. */
FVM_API FVM_Result fvm_debug_script_function_count(FVM_Context *, size_t *);


/* Implement debug script function at for this subsystem. */
FVM_API FVM_Result fvm_debug_script_function_at(FVM_Context *, size_t, const char **);


/* Return full borrowed registry names; views expire on registry mutation or destruction. */
FVM_API FVM_Result fvm_debug_native_function_at_bytes(FVM_Context *, size_t, const char **,
                                                      size_t *);


/* Return the counted script name using the same bounded registry snapshot order. */
FVM_API FVM_Result fvm_debug_script_function_at_bytes(FVM_Context *, size_t, const char **,
                                                      size_t *);
#ifdef __cplusplus
}
#endif
#endif
