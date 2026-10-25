/* Binary S-record request/reply envelopes; session identity comes from authenticated transport. */
#ifndef FLASHVM_RWIRE_H
#define FLASHVM_RWIRE_H /* Guard bounded native resource wire helpers. */
#include "revents.h"
#include "sbindings.h"
#define FVM_RWIRE_VERSION 1UL                             /* Payload schema version, encoded using canonical S integers. */
#define FVM_RWIRE_REQUEST_TYPE "flashvm.resource.request" /* Incoming binding identity. */
#define FVM_RWIRE_REPLY_TYPE \
    "flashvm.resource.reply"                              /* Outgoing/incoming reply binding identity. */
#define FVM_RWIRE_EVENT_TYPE "flashvm.resource.event"     /* Direction-local copied observer event. */
#define FVM_RWIRE_BODY_MAX \
    (FVM_R_BYTES + 80U)                                   /* Framing quota for payload plus bounded metadata. */
#ifdef __cplusplus
extern "C"
{
#endif


/* Encode version,id,target-slot,generation,operation,size,bytes under caller's nonzero bound
     * tag. request.session is deliberately absent from wire; caller/transport provides
     * authenticated identity. Outputs unchanged on failure. Return S-record status, no queue
     * mutation or VM execution. */
FVM_API FVM_SResult fvm_rrequest_record_write(unsigned long tag, const FVM_RRequest *,
                                              void *output, size_t capacity, size_t *used);


/* Verify CRC, incoming binding identity/version and whole request payload, then enqueue once.
     * Authenticated session is a separate trusted adapter argument. Tag zero/unbound/wrong schema
     * never runs resource code. PENDING means incomplete wire prefix; malformed/CRC data INVALID.
     * consumed changes only if queue post succeeds; on a known rejection the transport must
     * explicitly discard/answer its already framed packet, not spin retrying it. No auto
     * bindings/replay handling. */
FVM_API FVM_RResult fvm_rdispatch_post_record(FVM_RDispatch *, unsigned long session,
                                              const FVM_SBindings *incoming, const void *input,
                                              size_t size, size_t *consumed);


/* Encode version,request-id,handler-result,size,bytes; does not take/ack a queue result.
     * Caller retains the copied native reply to retry output on capacity/I/O failure. */
FVM_API FVM_SResult fvm_rreply_record_write(unsigned long tag, unsigned long request_id,
                                            const FVM_RReply *, void *output, size_t capacity,
                                            size_t *used);


/* Verify a bound reply and copy id/result/bytes only on OK; outputs unchanged otherwise.
     * Wrong binding/schema/payload INVALID, unbound tag UNKNOWN, framing errors preserve S status.
     */
FVM_API FVM_SResult fvm_rreply_record_read(const FVM_SBindings *incoming, const void *input,
                                           size_t size, unsigned long *request_id, FVM_RReply *,
                                           size_t *consumed);


/* Encode a copied event without consuming its subscription. Retry I/O from that retained copy.
     * Payload: version,target-slot,generation,sequence,kind,missed,size,bytes, using S integers. */
FVM_API FVM_SResult fvm_revent_record_write(unsigned long tag, const FVM_REvent *, void *,
                                            size_t capacity, size_t *used);


/* Verify CRC/schema and copy one complete event only on OK. Unknown binding UNKNOWN;
     * malformed fields INVALID; framing statuses propagate. No session/authority is encoded. */
FVM_API FVM_SResult fvm_revent_record_read(const FVM_SBindings *, const void *, size_t,
                                           FVM_REvent *, size_t *consumed);
#ifdef __cplusplus
}
#endif
#endif
