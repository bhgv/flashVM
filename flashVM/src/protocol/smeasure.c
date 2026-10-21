/* Exclusive unique-part counters for explicit host deadlines; no network or VM dependency. */
#include "flashvm/smeasure.h"
#include <string.h>
/* Reject invalid descriptors before indexing fixed state or computing any subtraction. */
static int measure_state(const FVM_SMeasure *s)
{
    return s && s->struct_size == sizeof(*s) && s->report.expected &&
           s->report.expected <= FVM_SMEASURE_PARTS && s->deadline_ms &&
           s->deadline_ms <= FVM_SRECORD_U32 && s->last_ms <= FVM_SRECORD_U32;
}
/* Initialize the whole observation atomically, keeping identity separate from packet contents. */
FVM_SResult fvm_smeasure_init(FVM_SMeasure *out, unsigned long stream, unsigned long epoch,
                              unsigned long window, unsigned long expected,
                              unsigned long deadline_ms)
{
    FVM_SMeasure next;
    if (!out || !stream || stream > FVM_SRECORD_U32 || !epoch || epoch > FVM_SRECORD_U32 ||
        !window || window > FVM_SRECORD_U32 || !expected || expected > FVM_SMEASURE_PARTS ||
        !deadline_ms || deadline_ms > FVM_SRECORD_U32)
        return FVM_S_INVALID;
    memset(&next, 0, sizeof(next));
    next.struct_size = sizeof(next);
    next.report.stream = stream;
    next.report.epoch = epoch;
    next.report.window = window;
    next.report.expected = expected;
    next.deadline_ms = deadline_ms;
    *out = next;
    return FVM_S_OK;
}
/* Categories form a monotone quality order; duplicate packets cannot inflate the sample. */
FVM_SResult fvm_smeasure_record(FVM_SMeasure *s, unsigned long part, unsigned int valid,
                                unsigned long elapsed_ms)
{
    size_t byte;
    unsigned int shift, previous, category, mask;
    if (!measure_state(s) || part >= s->report.expected || valid > FVM_SMEASURE_VALID ||
        elapsed_ms > FVM_SRECORD_U32)
        return FVM_S_INVALID;
    if (s->closed || elapsed_ms < s->last_ms)
        return FVM_S_CONFLICT;
    byte = (size_t)(part / FVM_SMEASURE_PER_BYTE);
    shift = (unsigned int)(part % FVM_SMEASURE_PER_BYTE) * FVM_SMEASURE_BITS;
    previous = (s->categories[byte] >> shift) & FVM_SMEASURE_MASK;
    category = !valid                         ? FVM_SMEASURE_DAMAGED
               : elapsed_ms <= s->deadline_ms ? FVM_SMEASURE_ON_TIME
                                              : FVM_SMEASURE_LATE;
    if (category > previous)
    {
        if (previous == FVM_SMEASURE_DAMAGED)
            --s->report.damaged;
        if (previous == FVM_SMEASURE_LATE)
            --s->report.late;
        if (category == FVM_SMEASURE_DAMAGED)
            ++s->report.damaged;
        if (category == FVM_SMEASURE_LATE)
            ++s->report.late;
        if (category == FVM_SMEASURE_ON_TIME)
            ++s->report.on_time;
        mask = FVM_SMEASURE_MASK << shift;
        s->categories[byte] = (unsigned char)((s->categories[byte] & ~mask) | (category << shift));
    }
    s->last_ms = elapsed_ms;
    return FVM_S_OK;
}
/* Freeze historical counts before the host changes receiver/profile/object ownership. */
FVM_SResult fvm_smeasure_finish(FVM_SMeasure *s, unsigned long elapsed_ms, unsigned long rtt_known,
                                unsigned long rtt_ms, FVM_SAdaptFeedback *out)
{
    FVM_SAdaptFeedback report;
    if (!measure_state(s) || !out || elapsed_ms > FVM_SRECORD_U32 || rtt_known > 1 ||
        rtt_ms > FVM_SRECORD_U32 || (!rtt_known && rtt_ms))
        return FVM_S_INVALID;
    if (s->closed || elapsed_ms < s->deadline_ms || elapsed_ms < s->last_ms)
        return FVM_S_CONFLICT;
    report = s->report;
    report.rtt_known = rtt_known;
    report.rtt_ms = rtt_ms;
    s->report = report;
    s->closed = 1;
    *out = report;
    return FVM_S_OK;
}

/* Compare metadata semantically; native padding never participates in identity. */
static int measure_media_same(const FVM_SMeasureMedia *s)
{
    const FVM_SMediaReceiver *r = s->receiver;
    return r && r->struct_size == sizeof(*r) && r->tag == s->tag &&
           r->profile.slot_bytes == s->profile.slot_bytes &&
           r->profile.part_bytes == s->profile.part_bytes && r->info.mode == s->info.mode &&
           r->info.stream == s->info.stream && r->info.epoch == s->info.epoch &&
           r->info.object == s->info.object && r->info.time_ms == s->info.time_ms &&
           r->info.total == s->info.total;
}
/* Use the receiver's public validation path before deriving bounded expected geometry. */
FVM_SResult fvm_smeasure_media_init(FVM_SMeasureMedia *out, FVM_SMediaReceiver *receiver,
                                    unsigned long window, unsigned long deadline_ms)
{
    FVM_SMeasureMedia next;
    FVM_SResult result;
    size_t ignored;
    if (!out || fvm_smedia_missing(receiver, 0, 0, 0, &ignored) != FVM_S_OK)
        return FVM_S_INVALID;
    if (receiver->received)
        return FVM_S_CONFLICT;
    memset(&next, 0, sizeof(next));
    result = fvm_smeasure_init(
        &next.measurement, receiver->info.stream, receiver->info.epoch, window,
        (receiver->info.total - 1) / receiver->profile.part_bytes + 1, deadline_ms);
    if (result != FVM_S_OK)
        return result;
    next.struct_size = sizeof(next);
    next.receiver = receiver;
    next.info = receiver->info;
    next.profile = receiver->profile;
    next.tag = receiver->tag;
    *out = next;
    return FVM_S_OK;
}
/* No data or statistics change until time, object identity, checksum and duplicate data agree. */
FVM_SResult fvm_smeasure_media_accept(FVM_SMeasureMedia *s, const void *input, size_t size,
                                      unsigned long elapsed_ms, int *complete)
{
    FVM_SMediaPiece piece;
    FVM_SResult result;
    int done;
    if (!s || s->struct_size != sizeof(*s) || !measure_state(&s->measurement) || !complete ||
        elapsed_ms > FVM_SRECORD_U32)
        return FVM_S_INVALID;
    if (s->measurement.closed || elapsed_ms < s->measurement.last_ms || !measure_media_same(s))
        return FVM_S_CONFLICT;
    result = fvm_smedia_read_profile(&s->profile, input, size, s->tag, &piece);
    if (result != FVM_S_OK)
        return result;
    result = fvm_smedia_accept(s->receiver, input, size, &done);
    if (result != FVM_S_OK)
        return result;
    /* A successful receiver acceptance proves the bounded index; earlier checks prove time. */
    fvm_smeasure_record(&s->measurement, piece.part, FVM_SMEASURE_VALID, elapsed_ms);
    *complete = done;
    return FVM_S_OK;
}
/* Closing uses only copied window state and never touches the borrowed image or receiver. */
FVM_SResult fvm_smeasure_media_finish(FVM_SMeasureMedia *s, unsigned long elapsed_ms,
                                      unsigned long rtt_known, unsigned long rtt_ms,
                                      FVM_SAdaptFeedback *out)
{
    if (!s || s->struct_size != sizeof(*s))
        return FVM_S_INVALID;
    return fvm_smeasure_finish(&s->measurement, elapsed_ms, rtt_known, rtt_ms, out);
}
