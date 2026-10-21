/* Resource envelopes carry typed metadata, not native structures, session claims or C pointers. */
#include "flashvm/rwire.h"
#include <string.h>
/* Use exactly the same canonical unsigned 32-bit integer representation for every metadata field.
 */
static size_t put_field(unsigned char *out, unsigned long value)
{
    FVM_SInteger number;
    size_t used = 0;
    number.high = 0;
    number.low = value;
    number.negative = 0;
    fvm_sinteger_write(&number, out, FVM_SRECORD_INTEGER_MAX, &used);
    return used;
}
/* Decode private cursor without publishing partial caller data. */
static int get_field(const unsigned char **cursor, size_t *left, unsigned long *out)
{
    FVM_SInteger number;
    size_t used;
    if (fvm_sinteger_read(*cursor, *left, &number, &used) != FVM_S_OK || number.high ||
        number.negative)
        return 0;
    *out = number.low;
    *cursor += used;
    *left -= used;
    return 1;
}
/* Native request identity/operation checks share the dispatcher's nonzero 32-bit domain. */
static int identity(unsigned long value)
{
    return value && value <= FVM_SRECORD_U32;
}
/* Resolve only an explicit, immutable direction-local schema declaration after CRC acceptance. */
static FVM_SResult typed_record(const FVM_SBindings *incoming, const void *input, size_t size,
                                const char *type, FVM_SRecord *record, size_t *used)
{
    FVM_SBinding binding;
    FVM_SResult result;
    result = fvm_srecord_read(input, size, FVM_RWIRE_BODY_MAX, record, used);
    if (result != FVM_S_OK)
        return result;
    result = fvm_sbindings_lookup(incoming, record->tag, &binding);
    if (result != FVM_S_OK)
        return result;
    if (binding.version != FVM_RWIRE_VERSION || binding.type_size != strlen(type) ||
        memcmp(binding.type, type, binding.type_size))
        return FVM_S_INVALID;
    return FVM_S_OK;
}
/* Small private scratch holds only bounded control data and metadata; no record-sized allocation.
 */
FVM_SResult fvm_rrequest_record_write(unsigned long tag, const FVM_RRequest *q, void *output,
                                      size_t capacity, size_t *used)
{
    unsigned char body[FVM_RWIRE_BODY_MAX];
    size_t n = 0;
    if (!q || !identity(tag) || !identity(q->id) || !identity(q->target.id) ||
        !identity(q->target.generation) || !identity(q->operation) || (!q->data && q->size))
        return FVM_S_INVALID;
    if (q->size > FVM_R_BYTES)
        return FVM_S_LIMIT;
    n += put_field(body + n, FVM_RWIRE_VERSION);
    n += put_field(body + n, q->id);
    n += put_field(body + n, q->target.id);
    n += put_field(body + n, q->target.generation);
    n += put_field(body + n, q->operation);
    n += put_field(body + n, (unsigned long)q->size);
    if (q->size)
        memcpy(body + n, q->data, q->size);
    return fvm_srecord_write(tag, body, n + q->size, (unsigned char *)output, capacity, used);
}
/* Framing/schema rejection precedes authorization and copying into the native command queue. */
FVM_RResult fvm_rdispatch_post_record(FVM_RDispatch *d, unsigned long session,
                                      const FVM_SBindings *incoming, const void *input, size_t size,
                                      size_t *consumed)
{
    FVM_SRecord record;
    FVM_SResult status;
    FVM_RRequest q;
    FVM_RResult result;
    const unsigned char *cursor;
    size_t left, used;
    unsigned long version, length;
    if (!d || !incoming || !consumed || !identity(session))
        return FVM_R_INVALID;
    status = typed_record(incoming, input, size, FVM_RWIRE_REQUEST_TYPE, &record, &used);
    if (status == FVM_S_MORE)
        return FVM_R_PENDING;
    if (status == FVM_S_LIMIT)
        return FVM_R_LIMIT;
    if (status == FVM_S_UNKNOWN)
        return FVM_R_UNKNOWN;
    if (status != FVM_S_OK)
        return FVM_R_INVALID;
    memset(&q, 0, sizeof(q));
    cursor = record.payload;
    left = record.size;
    if (!get_field(&cursor, &left, &version) || version != FVM_RWIRE_VERSION ||
        !get_field(&cursor, &left, &q.id) || !get_field(&cursor, &left, &q.target.id) ||
        !get_field(&cursor, &left, &q.target.generation) ||
        !get_field(&cursor, &left, &q.operation) || !get_field(&cursor, &left, &length) ||
        length != left || length > FVM_R_BYTES)
        return FVM_R_INVALID;
    q.session = session;
    q.size = (size_t)length;
    q.data = cursor;
    result = fvm_rdispatch_post(d, &q);
    if (result == FVM_R_OK)
        *consumed = used;
    return result;
}
/* The caller owns its native result and may retry serialization before transport acknowledgement.
 */
FVM_SResult fvm_rreply_record_write(unsigned long tag, unsigned long id, const FVM_RReply *reply,
                                    void *output, size_t capacity, size_t *used)
{
    unsigned char body[FVM_RWIRE_BODY_MAX];
    size_t n = 0;
    if (!reply || !identity(tag) || !identity(id) || reply->result < FVM_R_OK ||
        reply->result > FVM_R_HANDLER || reply->result == FVM_R_PENDING)
        return FVM_S_INVALID;
    if (reply->size > FVM_R_BYTES)
        return FVM_S_LIMIT;
    n += put_field(body + n, FVM_RWIRE_VERSION);
    n += put_field(body + n, id);
    n += put_field(body + n, (unsigned long)reply->result);
    n += put_field(body + n, (unsigned long)reply->size);
    if (reply->size)
        memcpy(body + n, reply->data, reply->size);
    return fvm_srecord_write(tag, body, n + reply->size, (unsigned char *)output, capacity, used);
}
/* Copy exactly the verified active bytes, with no native pointer/layout on the wire. */
FVM_SResult fvm_rreply_record_read(const FVM_SBindings *incoming, const void *input, size_t size,
                                   unsigned long *id, FVM_RReply *out, size_t *consumed)
{
    FVM_SRecord record;
    FVM_SResult status;
    FVM_RReply reply;
    unsigned long version, request, result, length;
    const unsigned char *cursor;
    size_t left, used;
    if (!incoming || !id || !out || !consumed)
        return FVM_S_INVALID;
    status = typed_record(incoming, input, size, FVM_RWIRE_REPLY_TYPE, &record, &used);
    if (status != FVM_S_OK)
        return status;
    cursor = record.payload;
    left = record.size;
    if (!get_field(&cursor, &left, &version) || version != FVM_RWIRE_VERSION ||
        !get_field(&cursor, &left, &request) || !identity(request) ||
        !get_field(&cursor, &left, &result) || result > FVM_R_HANDLER || result == FVM_R_PENDING ||
        !get_field(&cursor, &left, &length) || length != left || length > FVM_R_BYTES)
        return FVM_S_INVALID;
    memset(&reply, 0, sizeof(reply));
    reply.result = (FVM_RResult)result;
    reply.size = (size_t)length;
    if (length)
        memcpy(reply.data, cursor, (size_t)length);
    *id = request;
    *out = reply;
    *consumed = used;
    return FVM_S_OK;
}

/* A native take and transport acknowledgement are separate: encode a retained copied event. */
FVM_SResult fvm_revent_record_write(unsigned long tag, const FVM_REvent *event, void *output,
                                    size_t capacity, size_t *used)
{
    unsigned char body[FVM_RWIRE_BODY_MAX];
    size_t n = 0;
    if (!event || !identity(tag) || !identity(event->target.id) ||
        !identity(event->target.generation) || !identity(event->sequence) ||
        !identity(event->kind) || event->missed > FVM_SRECORD_U32)
        return FVM_S_INVALID;
    if (event->size > FVM_R_BYTES)
        return FVM_S_LIMIT;
    n += put_field(body + n, FVM_RWIRE_VERSION);
    n += put_field(body + n, event->target.id);
    n += put_field(body + n, event->target.generation);
    n += put_field(body + n, event->sequence);
    n += put_field(body + n, event->kind);
    n += put_field(body + n, event->missed);
    n += put_field(body + n, (unsigned long)event->size);
    if (event->size)
        memcpy(body + n, event->data, event->size);
    return fvm_srecord_write(tag, body, n + event->size, (unsigned char *)output, capacity, used);
}
/* Direction-local schema validation prevents interpreting an unrelated same-number tag. */
FVM_SResult fvm_revent_record_read(const FVM_SBindings *incoming, const void *input, size_t size,
                                   FVM_REvent *out, size_t *consumed)
{
    FVM_SRecord record;
    FVM_SResult status;
    FVM_REvent event;
    const unsigned char *cursor;
    size_t left, used;
    unsigned long version, length;
    if (!incoming || !out || !consumed)
        return FVM_S_INVALID;
    status = typed_record(incoming, input, size, FVM_RWIRE_EVENT_TYPE, &record, &used);
    if (status != FVM_S_OK)
        return status;
    memset(&event, 0, sizeof(event));
    cursor = record.payload;
    left = record.size;
    if (!get_field(&cursor, &left, &version) || version != FVM_RWIRE_VERSION ||
        !get_field(&cursor, &left, &event.target.id) || !identity(event.target.id) ||
        !get_field(&cursor, &left, &event.target.generation) ||
        !identity(event.target.generation) || !get_field(&cursor, &left, &event.sequence) ||
        !identity(event.sequence) || !get_field(&cursor, &left, &event.kind) ||
        !identity(event.kind) || !get_field(&cursor, &left, &event.missed) ||
        !get_field(&cursor, &left, &length) || length != left || length > FVM_R_BYTES)
        return FVM_S_INVALID;
    event.size = (size_t)length;
    if (length)
        memcpy(event.data, cursor, (size_t)length);
    *out = event;
    *consumed = used;
    return FVM_S_OK;
}
