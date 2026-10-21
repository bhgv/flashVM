/* Tag-zero size agreement is separate from immutable legacy bindings and from data transport. */
#include "flashvm/scontrol.h"
#include <string.h>
/* Enforce conservative fixed-slot reserves and native representability before any activation. */
static int control_limits(const FVM_SControlLimits *p)
{
    if (!p || !p->tag || p->tag > FVM_SRECORD_U32 || !p->record_limit ||
        p->record_limit > FVM_SRECORD_BODY_MAX || p->record_limit > (size_t)-1)
        return 0;
    if (!p->slot_bytes)
        return !p->part_bytes;
    return p->slot_bytes >= FVM_SSLOT_MIN_BYTES && p->slot_bytes <= FVM_SSLOT_MAX_BYTES &&
           p->slot_bytes <= (size_t)-1 && p->record_limit <= p->slot_bytes - FVM_SCONTROL_FRAMING &&
           p->record_limit > FVM_SCONTROL_METADATA && p->part_bytes &&
           p->part_bytes <= p->record_limit - FVM_SCONTROL_METADATA;
}
/* Equal semantic fields, with no dependence on native padding bytes. */
static int control_same(const FVM_SControlLimits *a, const FVM_SControlLimits *b)
{
    return a->tag == b->tag && a->record_limit == b->record_limit &&
           a->slot_bytes == b->slot_bytes && a->part_bytes == b->part_bytes;
}
/* An acknowledgement cannot change direction/tag, mode or enlarge any proposed quota. */
static int control_within(const FVM_SControlLimits *p, const FVM_SControlLimits *ceiling)
{
    return control_limits(p) && p->tag == ceiling->tag &&
           p->record_limit <= ceiling->record_limit &&
           ((!p->slot_bytes && !ceiling->slot_bytes) ||
            (p->slot_bytes && ceiling->slot_bytes && p->slot_bytes <= ceiling->slot_bytes &&
             p->part_bytes <= ceiling->part_bytes));
}
/* A native exchange owns no allocations; callers must not edit its private state. */
static int control_exchange(const FVM_SControlExchange *x)
{
    return x && x->struct_size == sizeof(*x) && control_limits(&x->ceiling);
}
/* Boundaries are either explicitly absent or complete portable media identities. */
static int control_boundary(const FVM_SControlBoundary *b)
{
    return b && ((!b->stream && !b->epoch && !b->object) ||
                 (b->stream && b->stream <= FVM_SRECORD_U32 && b->epoch &&
                  b->epoch <= FVM_SRECORD_U32 && b->object && b->object <= FVM_SRECORD_U32));
}
/* ACKs cannot silently move a media profile change to a different object or epoch. */
static int control_boundary_same(const FVM_SControlBoundary *a, const FVM_SControlBoundary *b)
{
    return a->stream == b->stream && a->epoch == b->epoch && a->object == b->object;
}
/* Distinguish offer and ACK variants without interpreting unknown service commands. */
static int control_offer(unsigned long command)
{
    return command == FVM_SCONTROL_OFFER || command == FVM_SCONTROL_OFFER_AT;
}
/* Only these commands may commit active data limits. */
static int control_ack(unsigned long command)
{
    return command == FVM_SCONTROL_ACCEPT || command == FVM_SCONTROL_ACCEPT_AT;
}
/* Native messages must contain a recognized command and exact Uint32 correlation. */
static int control_message(const FVM_SControlMessage *m)
{
    return m && (control_offer(m->command) || control_ack(m->command)) && m->id &&
           m->id <= FVM_SRECORD_U32 && control_limits(&m->limits) &&
           control_boundary(&m->boundary) &&
           ((m->command == FVM_SCONTROL_OFFER_AT || m->command == FVM_SCONTROL_ACCEPT_AT)
                ? m->boundary.stream != 0 && m->limits.slot_bytes != 0
                : m->boundary.stream == 0);
}
/* Append a canonical field into bounded local control-message scratch storage. */
static size_t control_put(unsigned char *out, unsigned long value)
{
    FVM_SInteger n;
    size_t used = 0;
    n.high = 0;
    n.low = value;
    n.negative = 0;
    fvm_sinteger_write(&n, out, FVM_SRECORD_INTEGER_MAX, &used);
    return used;
}
/* Read exactly a canonical nonnegative Uint32 service field. */
static int control_get(const unsigned char **p, size_t *left, unsigned long *value)
{
    FVM_SInteger n;
    size_t used;
    if (fvm_sinteger_read(*p, *left, &n, &used) != FVM_S_OK || n.high || n.negative)
        return 0;
    *p += used;
    *left -= used;
    *value = n.low;
    return 1;
}
/* Small service messages remain ordinary variable-length S-records, never padded media slots. */
FVM_SResult fvm_scontrol_write(const FVM_SControlMessage *m, void *output, size_t capacity,
                               size_t *used)
{
    unsigned char payload[FVM_SCONTROL_FIELDS * FVM_SRECORD_INTEGER_MAX];
    size_t n = 0;
    if (!control_message(m))
        return FVM_S_INVALID;
    n += control_put(payload + n, FVM_SCONTROL_VERSION);
    n += control_put(payload + n, m->command);
    n += control_put(payload + n, m->id);
    n += control_put(payload + n, m->limits.tag);
    n += control_put(payload + n, m->limits.record_limit);
    n += control_put(payload + n, m->limits.slot_bytes);
    n += control_put(payload + n, m->limits.part_bytes);
    n += control_put(payload + n, 0);
    if (m->boundary.stream)
    {
        n += control_put(payload + n, m->boundary.stream);
        n += control_put(payload + n, m->boundary.epoch);
        n += control_put(payload + n, m->boundary.object);
    }
    return fvm_srecord_write(FVM_SRECORD_CONTROL_TAG, payload, n, (unsigned char *)output, capacity,
                             used);
}
/* Unknown tag-zero subservices remain distinguishable from a malformed known size message. */
FVM_SResult fvm_scontrol_read(const void *input, size_t size, FVM_SControlMessage *out,
                              size_t *used)
{
    FVM_SRecord record;
    FVM_SControlMessage m;
    FVM_SResult result;
    unsigned long version, reserved;
    const unsigned char *p;
    size_t left, consumed;
    if (!out || !used)
        return FVM_S_INVALID;
    result = fvm_srecord_read(input, size, FVM_SRECORD_BODY_MAX, &record, &consumed);
    if (result != FVM_S_OK)
        return result;
    if (record.tag != FVM_SRECORD_CONTROL_TAG)
        return FVM_S_UNKNOWN;
    p = record.payload;
    left = record.size;
    if (!control_get(&p, &left, &version))
        return FVM_S_INVALID;
    if (version != FVM_SCONTROL_VERSION)
        return FVM_S_UNKNOWN;
    memset(&m, 0, sizeof(m));
    if (!control_get(&p, &left, &m.command))
        return FVM_S_INVALID;
    if (!control_offer(m.command) && !control_ack(m.command))
        return FVM_S_UNKNOWN;
    if (!control_get(&p, &left, &m.id) || !control_get(&p, &left, &m.limits.tag) ||
        !control_get(&p, &left, &m.limits.record_limit) ||
        !control_get(&p, &left, &m.limits.slot_bytes) ||
        !control_get(&p, &left, &m.limits.part_bytes) || !control_get(&p, &left, &reserved) ||
        reserved)
        return FVM_S_INVALID;
    if ((m.command == FVM_SCONTROL_OFFER_AT || m.command == FVM_SCONTROL_ACCEPT_AT) &&
        (!control_get(&p, &left, &m.boundary.stream) ||
         !control_get(&p, &left, &m.boundary.epoch) || !control_get(&p, &left, &m.boundary.object)))
        return FVM_S_INVALID;
    if (left || !control_message(&m))
        return FVM_S_INVALID;
    *out = m;
    *used = consumed;
    return FVM_S_OK;
}
/* Initialize no active agreement; a local capacity declaration alone never authorizes data. */
FVM_SResult fvm_scontrol_init(FVM_SControlExchange *out, const FVM_SControlLimits *ceiling)
{
    FVM_SControlExchange x;
    if (!out || !control_limits(ceiling))
        return FVM_S_INVALID;
    memset(&x, 0, sizeof(x));
    x.struct_size = sizeof(x);
    x.ceiling = *ceiling;
    *out = x;
    return FVM_S_OK;
}
/* Starting a pending proposal does not replace an existing committed configuration. */
FVM_SResult fvm_scontrol_propose_at(FVM_SControlExchange *x, unsigned long id,
                                    const FVM_SControlLimits *limits,
                                    const FVM_SControlBoundary *boundary, FVM_SControlMessage *out)
{
    FVM_SControlMessage offer;
    if (!control_exchange(x) || !out || !id || id > FVM_SRECORD_U32 || !control_limits(limits) ||
        !control_boundary(boundary) || (boundary->stream && !limits->slot_bytes))
        return FVM_S_INVALID;
    if (x->pending_id || id <= x->last_id)
        return FVM_S_CONFLICT;
    /* A fixed-capable ceiling also permits a compact offer within its record quota. */
    if (!(control_within(limits, &x->ceiling) ||
          (!limits->slot_bytes && limits->tag == x->ceiling.tag &&
           limits->record_limit <= x->ceiling.record_limit)))
        return FVM_S_LIMIT;
    offer.command = boundary->stream ? FVM_SCONTROL_OFFER_AT : FVM_SCONTROL_OFFER;
    offer.boundary = *boundary;
    x->pending_boundary = *boundary;
    offer.id = id;
    offer.limits = *limits;
    x->pending = *limits;
    x->last_id = x->pending_id = id;
    x->role = offer.command;
    *out = offer;
    return FVM_S_OK;
}
/* Select the smaller portable capacity; negotiation only reduces, never expands local storage. */
FVM_SResult fvm_scontrol_respond(FVM_SControlExchange *x, const FVM_SControlMessage *offer,
                                 FVM_SControlMessage *out)
{
    FVM_SControlMessage reply;
    FVM_SControlLimits *p = &reply.limits;
    if (!control_exchange(x) || !out || !control_message(offer) || !control_offer(offer->command))
        return FVM_S_INVALID;
    if (x->pending_id || offer->id <= x->last_id || offer->limits.tag != x->ceiling.tag)
        return FVM_S_CONFLICT;
    reply = *offer;
    reply.command = offer->boundary.stream ? FVM_SCONTROL_ACCEPT_AT : FVM_SCONTROL_ACCEPT;
    if (p->record_limit > x->ceiling.record_limit)
        p->record_limit = x->ceiling.record_limit;
    if (p->slot_bytes)
    {
        if (!x->ceiling.slot_bytes)
            return FVM_S_UNKNOWN;
        if (p->slot_bytes > x->ceiling.slot_bytes)
            p->slot_bytes = x->ceiling.slot_bytes;
        if (p->record_limit > p->slot_bytes - FVM_SCONTROL_FRAMING)
            p->record_limit = p->slot_bytes - FVM_SCONTROL_FRAMING;
        if (p->part_bytes > x->ceiling.part_bytes)
            p->part_bytes = x->ceiling.part_bytes;
        if (p->part_bytes > p->record_limit - FVM_SCONTROL_METADATA)
            p->part_bytes = p->record_limit - FVM_SCONTROL_METADATA;
    }
    if (!control_limits(p))
        return FVM_S_LIMIT;
    x->pending = *p;
    x->pending_boundary = offer->boundary;
    x->last_id = x->pending_id = reply.id;
    x->role = reply.command;
    *out = reply;
    return FVM_S_OK;
}
/* Commit correlation/quotas atomically; duplicate committed ACKs are safe and idempotent. */
FVM_SResult fvm_scontrol_commit(FVM_SControlExchange *x, const FVM_SControlMessage *ack)
{
    if (!control_exchange(x) || !control_message(ack) || !control_ack(ack->command))
        return FVM_S_INVALID;
    if (!x->pending_id)
        return x->active_id == ack->id && control_same(&x->active, &ack->limits) &&
                       control_boundary_same(&x->active_boundary, &ack->boundary)
                   ? FVM_S_OK
                   : FVM_S_CONFLICT;
    if (ack->id != x->pending_id || !control_boundary_same(&x->pending_boundary, &ack->boundary) ||
        (control_offer(x->role) && !control_within(&ack->limits, &x->pending)) ||
        (control_ack(x->role) && !control_same(&ack->limits, &x->pending)))
        return FVM_S_CONFLICT;
    x->active = ack->limits;
    x->active_boundary = ack->boundary;
    x->active_id = ack->id;
    x->pending_id = x->role = 0;
    memset(&x->pending, 0, sizeof(x->pending));
    memset(&x->pending_boundary, 0, sizeof(x->pending_boundary));
    return FVM_S_OK;
}
/* Cancel preserves the old active configuration and does not recycle request IDs. */
FVM_SResult fvm_scontrol_cancel(FVM_SControlExchange *x)
{
    if (!control_exchange(x))
        return FVM_S_INVALID;
    x->pending_id = x->role = 0;
    memset(&x->pending, 0, sizeof(x->pending));
    memset(&x->pending_boundary, 0, sizeof(x->pending_boundary));
    return FVM_S_OK;
}

/* Generic tag negotiation has no media activation boundary and requires host quiescence. */
FVM_SResult fvm_scontrol_propose(FVM_SControlExchange *x, unsigned long id,
                                 const FVM_SControlLimits *limits, FVM_SControlMessage *out)
{
    static const FVM_SControlBoundary none = {0, 0, 0};
    return fvm_scontrol_propose_at(x, id, limits, &none, out);
}
