/* FlashVM: Public embedding API declarations: debug.
 * File: include/flashvm/debug.h.
 */

#ifndef FLASHVM_DEBUG_H
/* Define FLASHVM DEBUG H constant or compilation switch. */
#define FLASHVM_DEBUG_H
#include "channel.h"
#include "table.h"
#ifdef __cplusplus
extern "C"
{
#endif
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
            offset; /* Byte offset identifying the instruction or data. */
        int state /* Current state of the resource. */,
            deadlocked; /* Whether the wait graph contains a detected deadlock. */
        unsigned long owner_line /* Source line where the resource was acquired. */,
            owner_offset; /* Bytecode offset where the resource was acquired. */
        char owner_source[256] /* Source name where the resource was acquired. */,
            owner_function[256]; /* Function that acquired the resource. */
        char source[256] /* Source name associated with this entry. */,
            function[256]; /* Function name associated with this source location. */
    } FVM_DebugMutex;
    /* A row per waiter, or one row with waiter=0 for an unawaited mutex.
       Exact coherent registry snapshot; insufficient capacity returns NOT_FOUND. */
    /* Capture snapshot of debug mutex. */
    FVM_API FVM_Result fvm_debug_mutex_snapshot(FVM_DebugMutex *, size_t, size_t *);
    /* Define FVM_DebugChannel struct and its representation. */
    typedef struct FVM_DebugChannel
    {
        unsigned long identity; /* Stable identity of this shared resource. */
        size_t
            pending /* Queued messages or complete unread file records; zero after file closure. */,
            capacity; /* Allocated storage limit; zero may denote an unbounded queue. */
        int closed /* Whether further operations have been closed. */,
            frozen; /* Whether debugger interception has frozen delivery. */
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
    /* Decode an immutable packet snapshot without dequeueing. Resource capabilities
       remain live shared handles; this does not freeze their native state. Local file indexes
       count unread records, and inspection restores the cursor even after malformed input. */
    /* Implement debug channel message for this subsystem. */
    FVM_API FVM_Result fvm_debug_channel_message(FVM_Context *, FVM_Channel *, size_t, FVM_Value *);
    /* Define FVM_DebugChannelWait struct and its representation. */
    typedef struct FVM_DebugChannelWait
    {
        unsigned long context /* Owning interpreter context. */,
            channel /* Shared channel handle; ownership follows the enclosing API. */,
            line /* Source line number associated with this entry. */,
            offset;  /* Byte offset identifying the instruction or data. */
        int sending; /* Whether the intercepted operation sends rather than receives. */
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
#ifdef __cplusplus
}
#endif
#endif
