/* Optional bulk adaptation based on explicit trusted feedback, independent of transport clocks. */
#include "flashvm/sadapt.h"
#include <string.h>
/* Every identity/counter is portable Uint32 and categories are exclusive before addition. */
static int adapt_feedback(const FVM_SAdaptFeedback *f)
{
    return f && f->stream && f->stream <= FVM_SRECORD_U32 && f->epoch &&
           f->epoch <= FVM_SRECORD_U32 && f->window && f->window <= FVM_SRECORD_U32 &&
           f->expected && f->expected <= FVM_SRECORD_U32 && f->on_time <= f->expected &&
           f->late <= f->expected - f->on_time &&
           f->damaged <= f->expected - f->on_time - f->late && f->rtt_known <= 1 &&
           f->rtt_ms <= FVM_SRECORD_U32 && (f->rtt_known || !f->rtt_ms);
}
/* Semantic comparison ignores native padding bytes. */
static int adapt_same(const FVM_SControlLimits *a, const FVM_SControlLimits *b)
{
    return a && a->tag == b->tag && a->record_limit == b->record_limit &&
           a->slot_bytes == b->slot_bytes && a->part_bytes == b->part_bytes;
}
/* Floor/ceiling expected*percent/100 without ever multiplying a full Uint32 count by 100. */
static unsigned long adapt_threshold(unsigned long expected, unsigned long percent, int ceiling)
{
    unsigned long tail = (expected % FVM_SADAPT_PERCENT) * percent;
    return (expected / FVM_SADAPT_PERCENT) * percent +
           (tail + (ceiling ? FVM_SADAPT_PERCENT - 1 : 0)) / FVM_SADAPT_PERCENT;
}
/* Numeric config checks precede any ladder dereference or copying. */
static int adapt_config(const FVM_SAdaptConfig *c)
{
    return c && c->struct_size == sizeof(*c) && c->profiles && c->count &&
           c->count <= FVM_SADAPT_PROFILES && c->initial < c->count && c->stream &&
           c->stream <= FVM_SRECORD_U32 && c->epoch && c->epoch <= FVM_SRECORD_U32 &&
           c->bad_percent && c->bad_percent <= FVM_SADAPT_PERCENT &&
           c->good_percent < c->bad_percent && c->bad_rtt_ms && c->bad_rtt_ms <= FVM_SRECORD_U32 &&
           c->good_rtt_ms < c->bad_rtt_ms && c->bad_windows && c->bad_windows <= FVM_SRECORD_U32 &&
           c->good_windows && c->good_windows <= FVM_SRECORD_U32 &&
           c->cooldown_windows <= FVM_SRECORD_U32 && c->min_expected &&
           c->min_expected <= FVM_SRECORD_U32;
}
/* Private state is stable-address caller storage; moving it requires reinitialization. */
static int adapt_state(const FVM_SAdapt *s)
{
    return s && s->struct_size == sizeof(*s) && s->config.profiles == s->profiles &&
           adapt_config(&s->config) && s->current < s->config.count &&
           (s->pending == FVM_SADAPT_NONE || s->pending < s->config.count);
}
/* Illustrative policy defaults are separate from link-specific capacity and identity. */
FVM_SResult fvm_sadapt_config_init(FVM_SAdaptConfig *out)
{
    FVM_SAdaptConfig c;
    if (!out)
        return FVM_S_INVALID;
    memset(&c, 0, sizeof(c));
    c.struct_size = sizeof(c);
    c.bad_percent = FVM_SADAPT_BAD_PERCENT;
    c.good_percent = FVM_SADAPT_GOOD_PERCENT;
    c.bad_rtt_ms = FVM_SADAPT_BAD_RTT;
    c.good_rtt_ms = FVM_SADAPT_GOOD_RTT;
    c.bad_windows = FVM_SADAPT_BAD_WINDOWS;
    c.good_windows = FVM_SADAPT_GOOD_WINDOWS;
    c.cooldown_windows = FVM_SADAPT_COOLDOWN_WINDOWS;
    c.min_expected = FVM_SADAPT_MIN_EXPECTED;
    *out = c;
    return FVM_S_OK;
}
/* Reuse the limits validator: each supported bulk entry must fit real caller-advertised storage. */
FVM_SResult fvm_sadapt_init(FVM_SAdapt *out, const FVM_SAdaptConfig *c)
{
    FVM_SControlExchange validate;
    FVM_SAdapt next;
    size_t i;
    if (!out || !adapt_config(c))
        return FVM_S_INVALID;
    for (i = 0; i < c->count; ++i)
        if (!c->profiles[i].slot_bytes ||
            fvm_scontrol_init(&validate, c->profiles + i) != FVM_S_OK ||
            (i && (c->profiles[i].tag != c->profiles[0].tag ||
                   c->profiles[i].slot_bytes <= c->profiles[i - 1].slot_bytes ||
                   c->profiles[i].part_bytes <= c->profiles[i - 1].part_bytes ||
                   c->profiles[i].record_limit <= c->profiles[i - 1].record_limit)))
            return FVM_S_INVALID;
    memset(&next, 0, sizeof(next));
    next.struct_size = sizeof(next);
    next.config = *c;
    for (i = 0; i < c->count; ++i)
        next.profiles[i] = c->profiles[i];
    next.current = c->initial;
    next.pending = FVM_SADAPT_NONE;
    *out = next;
    out->config.profiles = out->profiles;
    return FVM_S_OK;
}
/* A valid observation changes policy only after all checks, and never performs negotiation itself.
 */
FVM_SResult fvm_sadapt_observe(FVM_SAdapt *s, const FVM_SAdaptFeedback *f, FVM_SAdaptDecision *out)
{
    FVM_SAdapt next;
    FVM_SAdaptDecision decision;
    unsigned long unusable;
    int bad, good;
    if (!adapt_state(s) || !adapt_feedback(f) || !out)
        return FVM_S_INVALID;
    if (s->pending != FVM_SADAPT_NONE || f->stream != s->config.stream ||
        f->epoch != s->config.epoch || f->window <= s->last_window)
        return FVM_S_CONFLICT;
    next = *s;
    next.last_window = f->window;
    memset(&decision, 0, sizeof(decision));
    decision.current = decision.candidate = s->current;
    if (f->expected < s->config.min_expected)
    {
        decision.action = FVM_SADAPT_INSUFFICIENT;
        next.bad_run = next.good_run = 0;
    }
    else if (next.cooldown)
    {
        --next.cooldown;
        next.bad_run = next.good_run = 0;
        decision.reasons = FVM_SADAPT_COOLDOWN;
    }
    else
    {
        unusable = f->expected - f->on_time;
        if (unusable >= adapt_threshold(f->expected, s->config.bad_percent, 1))
            decision.reasons |= FVM_SADAPT_LOSS;
        if (f->rtt_known && f->rtt_ms >= s->config.bad_rtt_ms)
            decision.reasons |= FVM_SADAPT_LATENCY;
        bad = decision.reasons != 0;
        good = !bad && f->rtt_known && f->rtt_ms <= s->config.good_rtt_ms &&
               unusable <= adapt_threshold(f->expected, s->config.good_percent, 0);
        next.bad_run = bad ? next.bad_run + 1 : 0;
        next.good_run = good ? next.good_run + 1 : 0;
        if (next.bad_run >= s->config.bad_windows)
        {
            next.bad_run = next.good_run = 0;
            if (s->current)
            {
                decision.action = FVM_SADAPT_SHRINK;
                decision.candidate = s->current - 1;
            }
        }
        else if (next.good_run >= s->config.good_windows)
        {
            next.bad_run = next.good_run = 0;
            decision.reasons = FVM_SADAPT_STABLE;
            if (s->current + 1 < s->config.count)
            {
                decision.action = FVM_SADAPT_GROW;
                decision.candidate = s->current + 1;
            }
        }
        if (decision.action != FVM_SADAPT_KEEP)
            next.pending = decision.candidate;
    }
    decision.limits = s->profiles[decision.candidate];
    *s = next;
    s->config.profiles = s->profiles;
    *out = decision;
    return FVM_S_OK;
}
/* A network ACK may not silently substitute another unrepresented ladder profile. */
FVM_SResult fvm_sadapt_commit(FVM_SAdapt *s, const FVM_SControlLimits *selected)
{
    if (!adapt_state(s) || !selected)
        return FVM_S_INVALID;
    if (s->pending == FVM_SADAPT_NONE || !adapt_same(selected, s->profiles + s->pending))
        return FVM_S_CONFLICT;
    s->current = s->pending;
    s->pending = FVM_SADAPT_NONE;
    s->bad_run = s->good_run = 0;
    s->cooldown = s->config.cooldown_windows;
    return FVM_S_OK;
}
/* Rejected negotiation does not change the committed data geometry. */
FVM_SResult fvm_sadapt_reject(FVM_SAdapt *s)
{
    if (!adapt_state(s))
        return FVM_S_INVALID;
    if (s->pending == FVM_SADAPT_NONE)
        return FVM_S_CONFLICT;
    s->pending = FVM_SADAPT_NONE;
    s->bad_run = s->good_run = 0;
    s->cooldown = s->config.cooldown_windows;
    return FVM_S_OK;
}
/* Encode canonical portable service metadata, never native structs. */
static size_t adapt_put(unsigned char *out, unsigned long value)
{
    FVM_SInteger n;
    size_t used = 0;
    n.high = 0;
    n.low = value;
    n.negative = 0;
    fvm_sinteger_write(&n, out, FVM_SRECORD_INTEGER_MAX, &used);
    return used;
}
/* Read only canonical unsigned Uint32 fields into private decode scratch. */
static int adapt_get(const unsigned char **p, size_t *left, unsigned long *value)
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
/* Feedback remains a compact multifunctional tag-zero record, not a fixed data slot. */
FVM_SResult fvm_sadapt_feedback_write(const FVM_SAdaptFeedback *f, void *output, size_t capacity,
                                      size_t *used)
{
    unsigned char payload[FVM_SADAPT_FIELDS * FVM_SRECORD_INTEGER_MAX];
    size_t n = 0;
    if (!adapt_feedback(f))
        return FVM_S_INVALID;
    n += adapt_put(payload + n, FVM_SADAPT_SERVICE);
    n += adapt_put(payload + n, FVM_SADAPT_REPORT);
    n += adapt_put(payload + n, f->stream);
    n += adapt_put(payload + n, f->epoch);
    n += adapt_put(payload + n, f->window);
    n += adapt_put(payload + n, f->expected);
    n += adapt_put(payload + n, f->on_time);
    n += adapt_put(payload + n, f->late);
    n += adapt_put(payload + n, f->damaged);
    n += adapt_put(payload + n, f->rtt_known);
    n += adapt_put(payload + n, f->rtt_ms);
    return fvm_srecord_write(FVM_SRECORD_CONTROL_TAG, payload, n, (unsigned char *)output, capacity,
                             used);
}
/* Unknown services leave routing to the host, preserving legacy bindings and size agreement. */
FVM_SResult fvm_sadapt_feedback_read(const void *input, size_t size, FVM_SAdaptFeedback *out,
                                     size_t *used)
{
    FVM_SRecord record;
    FVM_SAdaptFeedback f;
    FVM_SResult result;
    unsigned long service, command;
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
    if (!adapt_get(&p, &left, &service))
        return FVM_S_INVALID;
    if (service != FVM_SADAPT_SERVICE)
        return FVM_S_UNKNOWN;
    if (!adapt_get(&p, &left, &command))
        return FVM_S_INVALID;
    if (command != FVM_SADAPT_REPORT)
        return FVM_S_UNKNOWN;
    memset(&f, 0, sizeof(f));
    if (!adapt_get(&p, &left, &f.stream) || !adapt_get(&p, &left, &f.epoch) ||
        !adapt_get(&p, &left, &f.window) || !adapt_get(&p, &left, &f.expected) ||
        !adapt_get(&p, &left, &f.on_time) || !adapt_get(&p, &left, &f.late) ||
        !adapt_get(&p, &left, &f.damaged) || !adapt_get(&p, &left, &f.rtt_known) ||
        !adapt_get(&p, &left, &f.rtt_ms) || left || !adapt_feedback(&f))
        return FVM_S_INVALID;
    *out = f;
    *used = consumed;
    return FVM_S_OK;
}
