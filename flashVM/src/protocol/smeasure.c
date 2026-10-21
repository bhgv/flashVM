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
    unsigned long window, unsigned long expected, unsigned long deadline_ms)
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
    category = !valid ? FVM_SMEASURE_DAMAGED : elapsed_ms <= s->deadline_ms ?
        FVM_SMEASURE_ON_TIME : FVM_SMEASURE_LATE;
    if (category > previous)
    {
        if (previous == FVM_SMEASURE_DAMAGED) --s->report.damaged;
        if (previous == FVM_SMEASURE_LATE) --s->report.late;
        if (category == FVM_SMEASURE_DAMAGED) ++s->report.damaged;
        if (category == FVM_SMEASURE_LATE) ++s->report.late;
        if (category == FVM_SMEASURE_ON_TIME) ++s->report.on_time;
        mask = FVM_SMEASURE_MASK << shift;
        s->categories[byte] = (unsigned char)((s->categories[byte] & ~mask) | (category << shift));
    }
    s->last_ms = elapsed_ms;
    return FVM_S_OK;
}
/* Freeze historical counts before the host changes receiver/profile/object ownership. */
FVM_SResult fvm_smeasure_finish(FVM_SMeasure *s, unsigned long elapsed_ms,
    unsigned long rtt_known, unsigned long rtt_ms, FVM_SAdaptFeedback *out)
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
