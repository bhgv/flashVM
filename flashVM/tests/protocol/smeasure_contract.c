/* Boundary, category-upgrade and failure-atomicity contract for bounded receive measurements. */
#include "flashvm/smeasure.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
        {                                                                                                              \
            fprintf(stderr, "line %d: %s\n", __LINE__, #x);                                                            \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)


/* Exercise distinct arrivals, repairs and a report frozen before late file recovery. */
int main(void)
{
    FVM_SMeasure s, saved;
    FVM_SAdaptFeedback report, untouched;
    unsigned char wire[128];
    size_t used;
    memset(&s, 0xa5, sizeof(s));
    saved = s;
    CHECK(fvm_smeasure_init(&s, 0, 1, 1, 6, 100) == FVM_S_INVALID);
    CHECK(!memcmp(&s, &saved, sizeof(s)));
    CHECK(fvm_smeasure_init(&s, 7, 1, 1, 6, 100) == FVM_S_OK);
    CHECK(fvm_smeasure_record(&s, 0, 0, 0) == FVM_S_OK);
    CHECK(s.report.damaged == 1);
    CHECK(fvm_smeasure_record(&s, 0, 0, 1) == FVM_S_OK);
    CHECK(fvm_smeasure_record(&s, 0, 1, 2) == FVM_S_OK);
    CHECK(s.report.damaged == 0 && s.report.on_time == 1);
    CHECK(fvm_smeasure_record(&s, 1, 1, 100) == FVM_S_OK);
    CHECK(fvm_smeasure_record(&s, 2, 0, 100) == FVM_S_OK);
    CHECK(fvm_smeasure_record(&s, 3, 0, 100) == FVM_S_OK);
    CHECK(fvm_smeasure_record(&s, 3, 1, 101) == FVM_S_OK);
    CHECK(fvm_smeasure_record(&s, 0, 1, 102) == FVM_S_OK);
    CHECK(fvm_smeasure_record(&s, 1, 0, 102) == FVM_S_OK);
    CHECK(s.report.on_time == 2 && s.report.late == 1 && s.report.damaged == 1);
    saved = s;
    CHECK(fvm_smeasure_record(&s, 4, 1, 101) == FVM_S_CONFLICT);
    CHECK(fvm_smeasure_record(&s, 6, 1, 102) == FVM_S_INVALID);
    CHECK(fvm_smeasure_record(&s, 4, 2, 102) == FVM_S_INVALID);
    CHECK(!memcmp(&s, &saved, sizeof(s)));
    memset(&report, 0xa5, sizeof(report));
    untouched = report;
    CHECK(fvm_smeasure_finish(&s, 99, 1, 20, &report) == FVM_S_CONFLICT);
    CHECK(fvm_smeasure_finish(&s, 101, 1, 20, &report) == FVM_S_CONFLICT);
    CHECK(fvm_smeasure_finish(&s, 102, 0, 20, &report) == FVM_S_INVALID);
    CHECK(!memcmp(&s, &saved, sizeof(s)) && !memcmp(&report, &untouched, sizeof(report)));
    CHECK(fvm_smeasure_finish(&s, 102, 1, 20, &report) == FVM_S_OK);
    CHECK(report.stream == 7 && report.epoch == 1 && report.window == 1 && report.expected == 6);
    CHECK(report.on_time == 2 && report.late == 1 && report.damaged == 1 && report.rtt_ms == 20);
    CHECK(fvm_sadapt_feedback_write(&report, wire, sizeof(wire), &used) == FVM_S_OK);
    saved = s;
    untouched = report;
    CHECK(fvm_smeasure_record(&s, 4, 1, 103) == FVM_S_CONFLICT);
    CHECK(fvm_smeasure_finish(&s, 103, 1, 20, &report) == FVM_S_CONFLICT);
    CHECK(!memcmp(&s, &saved, sizeof(s)) && !memcmp(&report, &untouched, sizeof(report)));
    CHECK(fvm_smeasure_init(
              &s, FVM_SRECORD_U32, FVM_SRECORD_U32, FVM_SRECORD_U32, FVM_SMEASURE_PARTS, FVM_SRECORD_U32) == FVM_S_OK);
    CHECK(fvm_smeasure_record(&s, FVM_SMEASURE_PARTS - 1, 1, FVM_SRECORD_U32) == FVM_S_OK);
    CHECK(fvm_smeasure_finish(&s, FVM_SRECORD_U32, 0, 0, &report) == FVM_S_OK && report.on_time == 1);
    puts("PASS bounded unique receive windows, repairs, deadlines and frozen reports");
    return 0;
}
