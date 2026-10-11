#ifndef FVM_INTERNAL_CHANNEL_PACKET_H
#define FVM_INTERNAL_CHANNEL_PACKET_H
#include "flashvm/channel.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Internal in-process packets own no VM pointers. Send transfers ownership
   only on success; mixed receive transfers packet/string ownership. */
FVM_Result fvm_channel_send_primitive(FVM_Channel *,const FVM_Value *,int);
FVM_Result fvm_channel_packet_send(FVM_Channel *,void *,void (*)(void *),int);
FVM_Result fvm_channel_receive_mixed(FVM_Channel *,FVM_Value *,void **,int);
#ifdef __cplusplus
}
#endif
#endif
