/* FlashVM: Public embedding API declarations: transfer.
 * File: include/flashvm/transfer.h.
 */

#ifndef FLASHVM_TRANSFER_H
/* Define FLASHVM TRANSFER H constant or compilation switch. */
#define FLASHVM_TRANSFER_H
#include "channel.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Snapshot supported object graphs (cycles and shared edges included) before
       enqueueing. No source VM pointers are retained. Native state and callbacks require registered
       codecs/identifiers; unsupported resources are rejected before sending. Mutex/channel
       capabilities share retained native handles through memory queues; local wrapper decorations
       are not copied. File/durable transfer of live handles and queue-owned channel capability
       cycles return FVM_UNSUPPORTED. Primitive messages use the same FIFO. blocking=0 returns
       FVM_NOT_FOUND on full/empty open queues. Received values belong to the destination context.
     */


/* Send channel value. */
FVM_API FVM_Result fvm_channel_send_value(FVM_Context *, FVM_Channel *, const FVM_Value *,
                                          int blocking);


/* Receive channel value. */
FVM_API FVM_Result fvm_channel_receive_value(FVM_Context *, FVM_Channel *, FVM_Value *,
                                             int blocking);
#ifdef __cplusplus
}
#endif
#endif
