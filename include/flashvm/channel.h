#ifndef FLASHVM_CHANNEL_H
#define FLASHVM_CHANNEL_H
#include "value.h"
#ifdef __cplusplus
extern "C" {
#endif
/* A thread-safe, FIFO, bidirectional in-process channel.
 * Messages are copied; object pointers are deliberately rejected.
 * A received string is owned by the channel and remains valid until
 * the next receive on that channel or destruction of the channel.
 * Do not destroy a channel while other threads are using it.
 */
typedef struct FVM_Channel FVM_Channel;
FVM_API FVM_Result fvm_channel_create(FVM_Channel **out_channel);
FVM_API void fvm_channel_close(FVM_Channel *channel);
/* Retain a shared handle before handing it to another owner or worker.
 * Each owner releases exactly once; destroy is an alias for release.
 * Last release must occur after all concurrent operations have stopped. */
FVM_API FVM_Result fvm_channel_retain(FVM_Channel *channel);
FVM_API void fvm_channel_release(FVM_Channel *channel);
FVM_API void fvm_channel_destroy(FVM_Channel *channel);
/* send and receive block until completed or closed; receive returns
 * FVM_ERROR on a closed and empty channel. */
FVM_API FVM_Result fvm_channel_send(FVM_Channel *channel, const FVM_Value *value);
FVM_API FVM_Result fvm_channel_receive(FVM_Channel *channel, FVM_Value *out_value);
/* Nonblocking receive: returns FVM_NOT_FOUND when empty and open;
 * FVM_ERROR when empty and closed. Received strings follow the same
 * borrowed-lifetime rule as fvm_channel_receive(). */
FVM_API FVM_Result fvm_channel_try_receive(FVM_Channel *channel, FVM_Value *out_value);
/* Nonblocking: available=0 means no message; a closed channel may still
 * contain unread messages. */
FVM_API FVM_Result fvm_channel_available(FVM_Channel *channel, int *available);
/* Queue capacity: 0 means unbounded (default). When full, send blocks until
 * a receiver removes an item or the channel is closed. Change capacity only
 * when no concurrent operations are active. Existing queued items are kept. */
FVM_API FVM_Result fvm_channel_set_capacity(FVM_Channel *channel, size_t capacity);
/* Snapshot of queued messages; safe during concurrent send/receive. */
FVM_API FVM_Result fvm_channel_pending(FVM_Channel *channel, size_t *pending);
#ifdef __cplusplus
}
#endif
#endif
