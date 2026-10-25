/* Independent UDP/TCP clocks and bounded authenticated advice; transport policy, not a socket
 * stack. */
#include "flashvm/rheartbeat.h"
#include <string.h>


/* Native IDs share a portable nonzero Uint32 domain. */
static int identity(unsigned long n)
{
    return n && n <= FVM_SRECORD_U32;
}


/* Configuration explicitly selects optional timers rather than guessing a NAT lifetime. */
static int policy_valid(const FVM_RHeartbeatPolicy *p)
{
    return p && p->struct_size == sizeof(*p) && p->transports && !(p->transports & ~FVM_RH_TRANSPORTS) &&
           !(p->allowed_hints & ~FVM_RH_HINTS) && p->udp_idle_ms <= FVM_RH_MAX_INTERVAL &&
           p->tcp_idle_ms <= FVM_RH_MAX_INTERVAL && p->request_gap_ms <= FVM_RH_MAX_INTERVAL &&
           p->udp_silence_ms <= FVM_RH_MAX_INTERVAL && p->tcp_silence_ms <= FVM_RH_MAX_INTERVAL &&
           (!p->allowed_hints || (p->transports == FVM_RH_TRANSPORTS && p->request_gap_ms));
}


/* Validate public native state before using counters in unsigned saturating arithmetic. */
static int valid(const FVM_RHeartbeat *h)
{
    return h && h->struct_size == sizeof(*h) && policy_valid(&h->policy) && identity(h->epoch) &&
           h->last_now <= FVM_SRECORD_U32 && h->udp_idle <= FVM_RH_MAX_INTERVAL && h->tcp_idle <= FVM_RH_MAX_INTERVAL &&
           h->udp_silence <= FVM_RH_MAX_INTERVAL && h->tcp_silence <= FVM_RH_MAX_INTERVAL &&
           h->request_age <= FVM_RH_MAX_INTERVAL && h->received_sequence <= FVM_SRECORD_U32 &&
           h->sent_sequence <= FVM_SRECORD_U32 && !(h->pending & ~h->policy.allowed_hints);
}


/* Saturate ages so a long silent connection does not look fresh after clock wrap. */
static unsigned long age(unsigned long value, unsigned long elapsed)
{
    return elapsed >= FVM_RH_MAX_INTERVAL - value ? FVM_RH_MAX_INTERVAL : value + elapsed;
}


/* Uint32 modular differences accept ordinary wrap and reject rollback/ambiguous long gaps. */
static FVM_RResult advance(FVM_RHeartbeat *h, unsigned long now)
{
    unsigned long elapsed;
    if (now > FVM_SRECORD_U32)
    {
        return FVM_R_INVALID;
    }
    elapsed = (now - h->last_now) & FVM_SRECORD_U32;
    if (elapsed > FVM_RH_MAX_INTERVAL)
    {
        return FVM_R_INVALID;
    }
    h->udp_idle = age(h->udp_idle, elapsed);
    h->tcp_idle = age(h->tcp_idle, elapsed);
    h->udp_silence = age(h->udp_silence, elapsed);
    h->tcp_silence = age(h->tcp_silence, elapsed);
    h->request_age = age(h->request_age, elapsed);
    h->last_now = now;
    return FVM_R_OK;
}


/* Successful init publishes one caller-owned native value, without retaining config storage. */
FVM_RResult
fvm_rheartbeat_init(FVM_RHeartbeat *h, const FVM_RHeartbeatPolicy *policy, unsigned long epoch, unsigned long now)
{
    FVM_RHeartbeat next;
    if (!h || !policy_valid(policy) || !identity(epoch) || now > FVM_SRECORD_U32)
    {
        return FVM_R_INVALID;
    }
    memset(&next, 0, sizeof(next));
    next.struct_size = sizeof(next);
    next.policy = *policy;
    next.epoch = epoch;
    next.last_now = now;
    *h = next;
    return FVM_R_OK;
}


/* Repeated polling keeps suggesting unsent work; only actual activity acknowledges a probe. */
FVM_RResult fvm_rheartbeat_due(FVM_RHeartbeat *h, unsigned long now, unsigned int *out)
{
    unsigned int actions = 0;
    if (!valid(h) || !out || advance(h, now) != FVM_R_OK)
    {
        return FVM_R_INVALID;
    }
    if (h->policy.transports & FVM_RH_UDP)
    {
        if (h->policy.udp_idle_ms && h->udp_idle >= h->policy.udp_idle_ms)
        {
            actions |= FVM_RH_SEND_UDP;
        }
        if (h->policy.udp_silence_ms && h->udp_silence >= h->policy.udp_silence_ms)
        {
            actions |= FVM_RH_UDP_STALE;
        }
    }
    if (h->policy.transports & FVM_RH_TCP)
    {
        if ((h->policy.tcp_idle_ms && h->tcp_idle >= h->policy.tcp_idle_ms) ||
            ((h->pending & FVM_RH_HINT_TCP) && h->request_age >= h->policy.request_gap_ms &&
             h->tcp_idle >= h->policy.request_gap_ms))
        {
            actions |= FVM_RH_SEND_TCP;
        }
        if (h->policy.tcp_silence_ms && h->tcp_silence >= h->policy.tcp_silence_ms)
        {
            actions |= FVM_RH_TCP_STALE;
        }
        if ((h->pending & FVM_RH_HINT_RECONNECT) && h->request_age >= h->policy.request_gap_ms)
        {
            actions |= FVM_RH_RECONNECT;
        }
    }
    *out = actions;
    return FVM_R_OK;
}


/* A sent heartbeat is not a received response; each transport retains its own silence clock. */
FVM_RResult fvm_rheartbeat_activity(FVM_RHeartbeat *h, unsigned int transport, int received, unsigned long now)
{
    if (!valid(h) || (transport != FVM_RH_UDP && transport != FVM_RH_TCP) || !(h->policy.transports & transport) ||
        (received != 0 && received != 1) || advance(h, now) != FVM_R_OK)
    {
        return FVM_R_INVALID;
    }
    if (transport == FVM_RH_UDP)
    {
        h->udp_idle = 0;
        if (received)
        {
            h->udp_silence = 0;
        }
    }
    else
    {
        h->tcp_idle = 0;
        h->request_age = 0;
        h->pending &= ~FVM_RH_HINT_TCP;
        if (received)
        {
            h->tcp_silence = 0;
        }
    }
    return FVM_R_OK;
}


/* Trusted ingress must authenticate outside this codec/policy before calling accept. */
FVM_RResult fvm_rheartbeat_accept(FVM_RHeartbeat *h, const FVM_RHeartbeatHint *hint, unsigned long now)
{
    if (!valid(h) || !hint || !identity(hint->epoch) || !identity(hint->sequence) || !hint->flags ||
        (hint->flags & ~FVM_RH_HINTS))
    {
        return FVM_R_INVALID;
    }
    if (hint->epoch != h->epoch || !(h->policy.transports & FVM_RH_UDP) || (hint->flags & ~h->policy.allowed_hints))
    {
        return FVM_R_DENIED;
    }
    if (hint->sequence <= h->received_sequence)
    {
        return FVM_R_CONFLICT;
    }
    if (advance(h, now) != FVM_R_OK)
    {
        return FVM_R_INVALID;
    }
    h->received_sequence = hint->sequence;
    h->pending |= hint->flags;
    h->udp_idle = 0;
    h->udp_silence = 0;
    return FVM_R_OK;
}


/* Handling/declining advice is distinct from successful network I/O. */
FVM_RResult fvm_rheartbeat_clear(FVM_RHeartbeat *h, unsigned int hints, unsigned long now)
{
    if (!valid(h) || !hints || (hints & ~FVM_RH_HINTS) || advance(h, now) != FVM_R_OK)
    {
        return FVM_R_INVALID;
    }
    h->pending &= ~hints;
    h->request_age = 0;
    return FVM_R_OK;
}


/* Exhaustion requests a new authenticated connection epoch rather than recycling replay IDs. */
FVM_RResult fvm_rheartbeat_next(FVM_RHeartbeat *h, unsigned int hints, FVM_RHeartbeatHint *out)
{
    FVM_RHeartbeatHint hint;
    if (!valid(h) || !out || !hints || (hints & ~FVM_RH_HINTS))
    {
        return FVM_R_INVALID;
    }
    if (h->sent_sequence == FVM_SRECORD_U32)
    {
        return FVM_R_LIMIT;
    }
    hint.epoch = h->epoch;
    hint.sequence = h->sent_sequence + 1;
    hint.flags = hints;
    h->sent_sequence = hint.sequence;
    *out = hint;
    return FVM_R_OK;
}


/* Canonical nonnegative integer fields use no native struct representation. */
static size_t put_field(unsigned char *p, unsigned long value)
{
    FVM_SInteger n;
    size_t used = 0;
    n.high = 0;
    n.low = value;
    n.negative = 0;
    fvm_sinteger_write(&n, p, FVM_SRECORD_INTEGER_MAX, &used);
    return used;
}


/* Parse one bounded unsigned field with no partially published output. */
static int get_field(const unsigned char **p, size_t *left, unsigned long *out)
{
    FVM_SInteger n;
    size_t used;
    if (fvm_sinteger_read(*p, *left, &n, &used) != FVM_S_OK || n.high || n.negative)
    {
        return 0;
    }
    *p += used;
    *left -= used;
    *out = n.low;
    return 1;
}


/* The checksum detects record errors only; a real adapter must add authenticated transport. */
FVM_SResult
fvm_rheartbeat_hint_write(unsigned long tag, const FVM_RHeartbeatHint *hint, void *out, size_t capacity, size_t *used)
{
    unsigned char body[FVM_RH_HINT_FIELDS * FVM_SRECORD_INTEGER_MAX];
    size_t n = 0;
    if (!hint || !identity(tag) || !identity(hint->epoch) || !identity(hint->sequence) || !hint->flags ||
        (hint->flags & ~FVM_RH_HINTS))
    {
        return FVM_S_INVALID;
    }
    n += put_field(body + n, FVM_RH_HINT_VERSION);
    n += put_field(body + n, hint->epoch);
    n += put_field(body + n, hint->sequence);
    n += put_field(body + n, hint->flags);
    return fvm_srecord_write(tag, body, n, (unsigned char *)out, capacity, used);
}


/* Verify directional schema after CRC acceptance; replay policy is a separate explicit step. */
FVM_SResult fvm_rheartbeat_hint_read(
    const FVM_SBindings *incoming, const void *input, size_t size, FVM_RHeartbeatHint *out, size_t *consumed)
{
    FVM_SRecord record;
    FVM_SBinding binding;
    FVM_SResult status;
    FVM_RHeartbeatHint hint;
    const unsigned char *p;
    size_t left, used;
    unsigned long version, flags;
    if (!incoming || !out || !consumed)
    {
        return FVM_S_INVALID;
    }
    status = fvm_srecord_read(input, size, FVM_RH_BODY_MAX, &record, &used);
    if (status != FVM_S_OK)
    {
        return status;
    }
    status = fvm_sbindings_lookup(incoming, record.tag, &binding);
    if (status != FVM_S_OK)
    {
        return status;
    }
    if (binding.version != FVM_RH_HINT_VERSION || binding.type_size != sizeof(FVM_RH_HINT_TYPE) - 1 ||
        memcmp(binding.type, FVM_RH_HINT_TYPE, sizeof(FVM_RH_HINT_TYPE) - 1))
    {
        return FVM_S_INVALID;
    }
    p = record.payload;
    left = record.size;
    if (!get_field(&p, &left, &version) || version != FVM_RH_HINT_VERSION || !get_field(&p, &left, &hint.epoch) ||
        !identity(hint.epoch) || !get_field(&p, &left, &hint.sequence) || !identity(hint.sequence) ||
        !get_field(&p, &left, &flags) || !flags || (flags & ~FVM_RH_HINTS) || left)
    {
        return FVM_S_INVALID;
    }
    hint.flags = (unsigned int)flags;
    *out = hint;
    *consumed = used;
    return FVM_S_OK;
}
