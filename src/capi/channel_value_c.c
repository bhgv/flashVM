#include "flashvm/channel_value.h"
FVM_Result fvm_channel_value_create(FVM_ChannelValue *out) {
    if (!out) return FVM_INVALID_ARGUMENT;
    out->channel=0;
    return fvm_channel_create(&out->channel);
}
FVM_Result fvm_channel_value_from_channel(FVM_ChannelValue *out, FVM_Channel *channel) {
    FVM_Result r;
    if (!out || !channel) return FVM_INVALID_ARGUMENT;
    r=fvm_channel_retain(channel);
    if (r==FVM_OK) out->channel=channel;
    return r;
}
FVM_Result fvm_channel_value_copy(FVM_ChannelValue *out, const FVM_ChannelValue *source) {
    if (!source) return FVM_INVALID_ARGUMENT;
    return fvm_channel_value_from_channel(out,source->channel);
}
void fvm_channel_value_release(FVM_ChannelValue *value) {
    if (!value || !value->channel) return;
    fvm_channel_release(value->channel);
    value->channel=0;
}
FVM_Result fvm_channel_value_send(const FVM_ChannelValue *value,const FVM_Value *message) {
    if (!value || !value->channel) return FVM_INVALID_ARGUMENT;
    return fvm_channel_send(value->channel,message);
}
FVM_Result fvm_channel_value_receive(const FVM_ChannelValue *value,FVM_Value *message) {
    if (!value || !value->channel) return FVM_INVALID_ARGUMENT;
    return fvm_channel_receive(value->channel,message);
}
