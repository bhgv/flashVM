/* FlashVM: Public embedding API declarations: channel.
 * File: include/flashvm/channel.h.
 */

#ifndef FLASHVM_CHANNEL_H
/* Define FLASHVM CHANNEL H constant or compilation switch. */
#define FLASHVM_CHANNEL_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* A thread-safe, FIFO, bidirectional in-process channel.
     * Messages are copied; object pointers are deliberately rejected.
     * A received string is owned by the channel and remains valid until
     * the next receive on that channel or destruction of the channel.
     * Do not destroy a channel while other threads are using it.
     */
    typedef struct FVM_Channel FVM_Channel;
    /* Wait directions are flags: both can occur on one bidirectional queue. */
#define FVM_CHANNEL_WAIT_SEND 1U
#define FVM_CHANNEL_WAIT_RECEIVE 2U
    /* One mutex-protected snapshot; no wait for data, capacity or a peer occurs.
     * It is observation, never a reservation; state can change immediately. */
    typedef struct FVM_ChannelWaitState
    {
        size_t pending;           /* Messages currently offered for reception. */
        size_t waiting_senders;   /* Threads waiting/polling for space or receipt. */
        size_t waiting_receivers; /* Threads waiting/polling for input or thaw. */
        unsigned int directions; /* SEND/RECEIVE flags from the two counts. */
        int closed;              /* Close has been requested. */
        int frozen;              /* Debugger currently suspends delivery. */
        int rendezvous;          /* Blocking send waits for actual receipt. */
    } FVM_ChannelWaitState;
    /* Local in-process channels only; file/remote endpoints return UNSUPPORTED.
     * Briefly takes the channel mutex. Errors leave out unchanged. */
    FVM_API FVM_Result fvm_channel_wait_state(FVM_Channel *, FVM_ChannelWaitState *out);
    /* Select rendezvous on an empty, idle, open local queue. Changing a live
     * queue is rejected. Buffered FIFO remains the compatibility default.
     * In rendezvous mode blocking send completes after receipt; try_send needs
     * an already waiting receiver. Close cancels an unreceived blocking send. */
    FVM_API FVM_Result fvm_channel_set_rendezvous(FVM_Channel *, int enabled);
    /* Implement channel identity for this subsystem. */
    FVM_API unsigned long fvm_channel_identity(FVM_Channel *);
    /* Create channel. */
    FVM_API FVM_Result fvm_channel_create(FVM_Channel **out_channel);
    /* File messages use the same versioned serialization as graph channels.
     * r opens existing files, w truncates, rw (also NULL) preserves/creates and
     * appends. Read and write positions are independent. Paths are UTF-8. */
    /* Open channel file. */
    FVM_API FVM_Result fvm_channel_open_file(const char *path, const char *mode, FVM_Channel **);
    /* Test whether channel file. */
    FVM_API FVM_Result fvm_channel_is_file(FVM_Channel *, int *);
    /* Raw file access uses those same positions. Do not interleave raw bytes and
     * structured messages unless the bytes form complete serialized records. */
    /* Read channel file. */
    FVM_API FVM_Result fvm_channel_file_read(FVM_Channel *, void *, size_t, size_t *);
    /* Write channel file. */
    FVM_API FVM_Result fvm_channel_file_write(FVM_Channel *, const void *, size_t);
    /* Test whether channel closed. */
    FVM_API FVM_Result fvm_channel_is_closed(FVM_Channel *, int *);
    /* Close channel. */
    FVM_API void fvm_channel_close(FVM_Channel *channel);
    /* Retain a shared handle before handing it to another owner or worker.
     * Each owner releases exactly once; destroy is an alias for release.
     * Last release must occur after all concurrent operations have stopped. */
    /* Retain channel. */
    FVM_API FVM_Result fvm_channel_retain(FVM_Channel *channel);
    /* Release channel. */
    FVM_API void fvm_channel_release(FVM_Channel *channel);
    /* Destroy channel. */
    FVM_API void fvm_channel_destroy(FVM_Channel *channel);
    /* send and receive block until completed or closed; receive returns
     * FVM_ERROR on a closed and empty channel. */
    /* Send channel. */
    FVM_API FVM_Result fvm_channel_send(FVM_Channel *channel, const FVM_Value *value);
    /* Receive channel. */
    FVM_API FVM_Result fvm_channel_receive(FVM_Channel *channel, FVM_Value *out_value);
    /* Nonblocking send: FVM_NOT_FOUND when full, FVM_ERROR when closed. */
    /* Attempt channel send. */
    FVM_API FVM_Result fvm_channel_try_send(FVM_Channel *, const FVM_Value *);
    /* Transfers ownership of any string to the receiver. Safe for multiple
       consumers; unlike borrowed receive, another receive cannot invalidate it.
       blocking=0 returns FVM_NOT_FOUND if empty/open. Release exactly once. */
    /* Receive channel owned. */
    FVM_API FVM_Result fvm_channel_receive_owned(FVM_Channel *, FVM_Value *, int blocking);
    /* Release channel message. */
    FVM_API void fvm_channel_message_release(FVM_Value *);
    /* Nonblocking receive: returns FVM_NOT_FOUND when empty and open;
     * FVM_ERROR when empty and closed. Received strings follow the same
     * borrowed-lifetime rule as fvm_channel_receive(). */
    /* Attempt channel receive. */
    FVM_API FVM_Result fvm_channel_try_receive(FVM_Channel *channel, FVM_Value *out_value);
    /* Nonblocking: available=0 means no message; a closed channel may still
     * contain unread messages. */
    /* Implement channel available for this subsystem. */
    FVM_API FVM_Result fvm_channel_available(FVM_Channel *channel, int *available);
    /* Queue capacity: 0 means unbounded (default). When full, send blocks until
     * a receiver removes an item or the channel is closed. Change capacity only
     * when no concurrent operations are active. Existing queued items are kept. */
    /* Update channel capacity. */
    FVM_API FVM_Result fvm_channel_set_capacity(FVM_Channel *channel, size_t capacity);
    /* Snapshot of queued messages; safe during concurrent send/receive. */
    /* Implement channel pending for this subsystem. */
    FVM_API FVM_Result fvm_channel_pending(FVM_Channel *channel, size_t *pending);
#ifdef __cplusplus
}
#endif
#endif
