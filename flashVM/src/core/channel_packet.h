/* FlashVM: Interpreter state, object lifetimes and shared runtime helpers: channel packet.
 * File: src/core/channel_packet.h.
 */

#ifndef FVM_INTERNAL_CHANNEL_PACKET_H
/* Define FVM INTERNAL CHANNEL PACKET H constant or compilation switch. */
#define FVM_INTERNAL_CHANNEL_PACKET_H
#include "flashvm/channel.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Remote operation codes: 0 status, 4 close, 5 send graph, 6 receive graph,
       7 debugger freeze/resume, 8 debugger state snapshot, 9 indexed debugger message copy. */
    typedef FVM_Result (*FVM_RemoteChannelFn)(void *, FVM_Context *, int, const unsigned char *,
                                              size_t, unsigned char **, size_t *, int);
    /* Create channel remote. */
    FVM_Result fvm_channel_create_remote(void *, FVM_RemoteChannelFn, void (*)(void *),
                                         const char *, FVM_Channel **);
    /* Return the path of the file backing this channel. */
    const char *fvm_channel_file_path(FVM_Channel *);
    /* Return the remote endpoint address associated with this channel. */
    const char *fvm_channel_remote_address(FVM_Channel *);
    /* Test whether channel stream. */
    FVM_Result fvm_channel_is_stream(FVM_Channel *, int *);
    /* Send channel stream. */
    FVM_Result fvm_channel_stream_send(FVM_Context *, FVM_Channel *, const unsigned char *, size_t);
    /* Receive channel stream. */
    FVM_Result fvm_channel_stream_receive(FVM_Context *, FVM_Channel *, unsigned char **, size_t *,
                                          int);
    /* Copy an indexed stream record locally or by remote opcode 9; caller frees data. */
    FVM_Result fvm_channel_stream_peek(FVM_Context *, FVM_Channel *, size_t, unsigned char **,
                                       size_t *);
    /* Allocate a process-local shared resource identity under the registry lock. */
    unsigned long fvm_sync_next_identity(void);
    /* Internal in-process packets own no VM pointers. Send transfers ownership
       only on success; mixed receive transfers packet/string ownership. */
    /* Send channel primitive. */
    FVM_Result fvm_channel_send_primitive(FVM_Channel *, const FVM_Value *, int);
    /* Send channel packet. */
    FVM_Result fvm_channel_packet_send(FVM_Channel *, void *, void (*)(void *),
                                       FVM_Result (*)(void *), int);
    /* Receive channel mixed. */
    FVM_Result fvm_channel_receive_mixed(FVM_Channel *, FVM_Value *, void **, int);
    /* Implement channel peek mixed for this subsystem. */
    FVM_Result fvm_channel_peek_mixed(FVM_Channel *, size_t, FVM_Value *, void **);
    /* Serialized file record transport; caller frees received buffers. */
    /* Send channel file record. */
    FVM_Result fvm_channel_file_send_record(FVM_Channel *, const unsigned char *, size_t);
    /* Receive channel file record. */
    FVM_Result fvm_channel_file_receive_record(FVM_Channel *, unsigned char **, size_t *, int);
    /* Copy an indexed local file record without advancing its read position; caller frees data. */
    FVM_Result fvm_channel_file_peek_record(FVM_Channel *, size_t, unsigned char **, size_t *);
    /* Encode serialization primitive. */
    FVM_Result fvm_serialization_encode_primitive(const FVM_Value *, unsigned char **, size_t *);
    /* Decode serialization primitive. */
    FVM_Result fvm_serialization_decode_primitive(const void *, size_t, FVM_Value *);
#ifdef __cplusplus
}
#endif
#endif
