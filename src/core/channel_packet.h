#ifndef FVM_INTERNAL_CHANNEL_PACKET_H
#define FVM_INTERNAL_CHANNEL_PACKET_H
#include "flashvm/channel.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Remote operation codes: 0 status, 4 close, 5 send graph, 6 receive graph. */
    typedef FVM_Result (*FVM_RemoteChannelFn)(void *, FVM_Context *, int, const unsigned char *,
                                              size_t, unsigned char **, size_t *, int);
    FVM_Result fvm_channel_create_remote(void *, FVM_RemoteChannelFn, void (*)(void *),
                                         const char *, FVM_Channel **);
    const char *fvm_channel_file_path(FVM_Channel *);
    const char *fvm_channel_remote_address(FVM_Channel *);
    FVM_Result fvm_channel_is_stream(FVM_Channel *, int *);
    FVM_Result fvm_channel_stream_send(FVM_Context *, FVM_Channel *, const unsigned char *, size_t);
    FVM_Result fvm_channel_stream_receive(FVM_Context *, FVM_Channel *, unsigned char **, size_t *,
                                          int);
    unsigned long fvm_sync_next_identity(void);
    /* Internal in-process packets own no VM pointers. Send transfers ownership
       only on success; mixed receive transfers packet/string ownership. */
    FVM_Result fvm_channel_send_primitive(FVM_Channel *, const FVM_Value *, int);
    FVM_Result fvm_channel_packet_send(FVM_Channel *, void *, void (*)(void *),
                                       FVM_Result (*)(void *), int);
    FVM_Result fvm_channel_receive_mixed(FVM_Channel *, FVM_Value *, void **, int);
    FVM_Result fvm_channel_peek_mixed(FVM_Channel *, size_t, FVM_Value *, void **);
    /* Serialized file record transport; caller frees received buffers. */
    FVM_Result fvm_channel_file_send_record(FVM_Channel *, const unsigned char *, size_t);
    FVM_Result fvm_channel_file_receive_record(FVM_Channel *, unsigned char **, size_t *, int);
    FVM_Result fvm_serialization_encode_primitive(const FVM_Value *, unsigned char **, size_t *);
    FVM_Result fvm_serialization_decode_primitive(const void *, size_t, FVM_Value *);
#ifdef __cplusplus
}
#endif
#endif
