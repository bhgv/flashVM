#ifndef FVM_INTERNAL_CHANNEL_PACKET_H
#define FVM_INTERNAL_CHANNEL_PACKET_H
#include "flashvm/channel.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Internal in-process packets own no VM pointers. Send transfers ownership
       only on success; mixed receive transfers packet/string ownership. */
    FVM_Result fvm_channel_send_primitive(FVM_Channel *, const FVM_Value *, int);
    FVM_Result fvm_channel_packet_send(FVM_Channel *, void *, void (*)(void *), int);
    FVM_Result fvm_channel_receive_mixed(FVM_Channel *, FVM_Value *, void **, int);
    /* Serialized file record transport; caller frees received buffers. */
    FVM_Result fvm_channel_file_send_record(FVM_Channel *, const unsigned char *, size_t);
    FVM_Result fvm_channel_file_receive_record(FVM_Channel *, unsigned char **, size_t *, int);
    FVM_Result fvm_serialization_encode_primitive(const FVM_Value *, unsigned char **, size_t *);
    FVM_Result fvm_serialization_decode_primitive(const void *, size_t, FVM_Value *);
#ifdef __cplusplus
}
#endif
#endif
