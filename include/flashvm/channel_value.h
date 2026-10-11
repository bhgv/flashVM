#ifndef FLASHVM_CHANNEL_VALUE_H
#define FLASHVM_CHANNEL_VALUE_H
#include "channel.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Explicitly owned channel-valued payload. Unlike borrowed FVM_Value,
 * this handle must be released once for every successful copy/retain.
 * Never use an FVM_ChannelValue after releasing its final reference. */
typedef struct FVM_ChannelValue {
    FVM_Channel *channel;
} FVM_ChannelValue;
FVM_API FVM_Result fvm_channel_value_create(FVM_ChannelValue *out);
FVM_API FVM_Result fvm_channel_value_from_channel(FVM_ChannelValue *out, FVM_Channel *channel);
FVM_API FVM_Result fvm_channel_value_copy(FVM_ChannelValue *out, const FVM_ChannelValue *source);
FVM_API void fvm_channel_value_release(FVM_ChannelValue *value);
FVM_API FVM_Result fvm_channel_value_send(const FVM_ChannelValue *value, const FVM_Value *message);
FVM_API FVM_Result fvm_channel_value_receive(const FVM_ChannelValue *value, FVM_Value *message);
#ifdef __cplusplus
}
#endif
#endif
