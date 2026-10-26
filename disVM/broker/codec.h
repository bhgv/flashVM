/* BD1 diagnostic records: canonical ASCII hex scalars and hex-encoded UTF-8 text. */
#ifndef DVM_BROKER_CODEC_H
#define DVM_BROKER_CODEC_H
#include "broker.h"
#define DVM_BROKER_FRAME_MAX 8192 /* Hard bound for one diagnostic frame; ordinary RPC results use another path. */
#ifdef __cplusplus
extern "C"
{
#endif


/* Encode the public diagnostic view into borrowed storage; size excludes any terminator.
 * No pointers/native struct padding are transmitted. Errors preserve storage and *size.
 * Output includes copied source/endpoint metadata, but not private heartbeat/deadline policy. */
DVM_BrokerResult dvm_broker_encode(const DVM_BrokerEvent *event, char *storage, size_t capacity, size_t *size);


/* Decode one complete counted frame; errors preserve *out. No VM/channel operation occurs.
 * Wire IDs/timestamps remain exact uint64 values. Non-wire native policy fields become zero.
 * Decoder validates bounded canonical hex, UTF-8, enum ranges, source extents and no trailing bytes. */
DVM_BrokerResult dvm_broker_decode(const char *frame, size_t size, DVM_BrokerEvent *out);
#ifdef __cplusplus
}
#endif
#endif
