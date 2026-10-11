#ifndef FLASHVM_DEBUG_H
#define FLASHVM_DEBUG_H
#include "table.h"
#include "channel.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Source mapping supplied by the embedding application; AVM1 updates offset.
       Unknown source/line is empty/zero. Copied snapshots contain no VM pointers. */
    FVM_API FVM_Result fvm_debug_set_location(FVM_Context *, const char *, unsigned long);
    /* Offsets are relative to the named function's AVM1 action block. Empty function
       names describe top-level bytecode. The nearest preceding entry maps an action. */
    FVM_API FVM_Result fvm_debug_map_action(FVM_Context *, const char *function,
                                            unsigned long offset, const char *source,
                                            unsigned long line);
    typedef struct FVM_DebugMutex
    {
        unsigned long identity, origin, owner, waiter, line, offset;
        int state, deadlocked;
        unsigned long owner_line, owner_offset;
        char owner_source[256], owner_function[256];
        char source[256], function[256];
    } FVM_DebugMutex;
    /* A row per waiter, or one row with waiter=0 for an unawaited mutex.
       Exact coherent registry snapshot; insufficient capacity returns NOT_FOUND. */
    FVM_API FVM_Result fvm_debug_mutex_snapshot(FVM_DebugMutex *, size_t, size_t *);
    typedef struct FVM_DebugChannel
    {
        unsigned long identity;
        size_t pending, capacity;
        int closed, frozen;
    } FVM_DebugChannel;
    /* Freeze dequeue only; producers can still enqueue. Closed frozen queues require
       explicit resume to drain. File-channel interception is not implemented. */
    FVM_API FVM_Result fvm_debug_channel_freeze(FVM_Channel *, int);
    FVM_API FVM_Result fvm_debug_channel_state(FVM_Channel *, FVM_DebugChannel *);
    /* Decode an immutable packet snapshot without dequeueing. Resource capabilities
       remain live shared handles; this does not freeze their native state. */
    FVM_API FVM_Result fvm_debug_channel_message(FVM_Context *, FVM_Channel *, size_t, FVM_Value *);
    typedef struct FVM_DebugChannelWait
    {
        unsigned long context, channel, line, offset;
        int sending;
        char source[256], function[256];
    } FVM_DebugChannelWait;
    /* Active ActionScript send/receive waits, not inferred channel deadlocks. */
    FVM_API FVM_Result fvm_debug_channel_wait_snapshot(FVM_DebugChannelWait *, size_t, size_t *);
    /* Bounded oldest-to-newest error intercept queue. Events copy locations and
       addresses. Overflow drops oldest events; clear only after successful copy. */
    typedef struct FVM_DebugResourceError
    {
        unsigned long context, resource, line, offset;
        FVM_Result result;
        char operation[32], address[256], source[256], function[256];
    } FVM_DebugResourceError;
    FVM_API FVM_Result fvm_debug_resource_errors(FVM_DebugResourceError *, size_t, size_t *,
                                                 int clear);
    /* Snapshot-style inspection: borrowed handles; only call on the VM thread. */
    FVM_API FVM_Result fvm_debug_object_count(FVM_Context *, size_t *);
    FVM_API FVM_Result fvm_debug_object_at(FVM_Context *, size_t, FVM_Object **);
    FVM_API FVM_Result fvm_debug_native_function_count(FVM_Context *, size_t *);
    FVM_API FVM_Result fvm_debug_native_function_at(FVM_Context *, size_t, const char **);
    FVM_API FVM_Result fvm_debug_script_function_count(FVM_Context *, size_t *);
    FVM_API FVM_Result fvm_debug_script_function_at(FVM_Context *, size_t, const char **);
#ifdef __cplusplus
}
#endif
#endif
