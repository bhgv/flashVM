/* Actual verified fragments feed unique statistics; failed slots and later repairs stay atomic. */
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


/* Validate receiver binding, corrupt/conflicting data, late completion and frozen feedback. */
int main(void)
{
    FVM_SMediaProfile profile = {256, 128};
    FVM_SMediaInfo info = {FVM_SMEDIA_FILE, 7, 1, 9, 0, 300};
    FVM_SMediaReceiver receiver, old_receiver;
    FVM_SMeasureMedia state, before, other;
    FVM_SAdaptFeedback report;
    unsigned char image[300], saved[300], data[300], slots[3][256], bad[256];
    unsigned long part;
    size_t used, size;
    int complete = 77;
    memset(data, 0x5a, sizeof(data));
    memset(image, 0xa5, sizeof(image));
    CHECK(fvm_smedia_receiver_init_profile(&receiver, &profile, 19, &info, image, sizeof(image)) == FVM_S_OK);
    CHECK(fvm_smeasure_media_init(&state, &receiver, 1, 100) == FVM_S_OK);
    CHECK(state.measurement.report.expected == 3);
    for (part = 0; part < 3; ++part)
    {
        size = part == 2 ? 44 : 128;
        CHECK(fvm_smedia_write_profile(&profile, 19, &info, part, data + part * 128, size, slots[part], 256, &used) ==
              FVM_S_OK);
    }
    CHECK(fvm_smeasure_media_accept(&state, slots[1], 256, 10, &complete) == FVM_S_OK && !complete);
    CHECK(fvm_smeasure_media_accept(&state, slots[1], 256, 11, &complete) == FVM_S_OK);
    CHECK(state.measurement.report.on_time == 1 && receiver.received == 1);
    CHECK(fvm_smeasure_media_init(&other, &receiver, 2, 100) == FVM_S_CONFLICT);
    before = state;
    old_receiver = receiver;
    memcpy(saved, image, sizeof(image));
    complete = 77;
    memcpy(bad, slots[0], 256);
    bad[0] ^= 1;
    CHECK(fvm_smeasure_media_accept(&state, bad, 256, 12, &complete) != FVM_S_OK);
    data[128] ^= 1;
    CHECK(fvm_smedia_write_profile(&profile, 19, &info, 1, data + 128, 128, bad, 256, &used) == FVM_S_OK);
    CHECK(fvm_smeasure_media_accept(&state, bad, 256, 12, &complete) == FVM_S_CONFLICT);
    CHECK(fvm_smeasure_media_accept(&state, slots[0], 256, 10, &complete) == FVM_S_CONFLICT);
    CHECK(!memcmp(&state, &before, sizeof(state)) && !memcmp(&receiver, &old_receiver, sizeof(receiver)));
    CHECK(!memcmp(image, saved, sizeof(image)) && complete == 77);
    CHECK(fvm_smeasure_media_accept(&state, slots[0], 256, 101, &complete) == FVM_S_OK && !complete);
    CHECK(state.measurement.report.on_time == 1 && state.measurement.report.late == 1);
    CHECK(fvm_smeasure_media_finish(&state, 101, 1, 20, &report) == FVM_S_OK);
    CHECK(report.expected == 3 && report.on_time == 1 && report.late == 1 && report.damaged == 0);
    before = state;
    complete = 77;
    CHECK(fvm_smeasure_media_accept(&state, slots[2], 256, 102, &complete) == FVM_S_CONFLICT && complete == 77);
    CHECK(fvm_smedia_accept(&receiver, slots[2], 256, &complete) == FVM_S_OK && complete);
    CHECK(!memcmp(&state, &before, sizeof(state)) && report.on_time == 1);
    data[128] ^= 1;
    CHECK(!memcmp(image, data, sizeof(image)));
    CHECK(fvm_smedia_receiver_init_profile(&receiver, &profile, 19, &info, image, sizeof(image)) == FVM_S_OK);
    CHECK(fvm_smeasure_media_init(&state, &receiver, 2, 100) == FVM_S_OK);
    ++info.object;
    CHECK(fvm_smedia_receiver_init_profile(&receiver, &profile, 19, &info, image, sizeof(image)) == FVM_S_OK);
    before = state;
    CHECK(fvm_smeasure_media_accept(&state, slots[0], 256, 0, &complete) == FVM_S_CONFLICT);
    CHECK(!memcmp(&state, &before, sizeof(state)));
    CHECK(fvm_smeasure_media_init(&state, &receiver, 3, 100) == FVM_S_OK);
    info.object = 9;
    profile.part_bytes = 100;
    CHECK(fvm_smedia_receiver_init_profile(&receiver, &profile, 19, &info, image, sizeof(image)) == FVM_S_OK);
    CHECK(fvm_smeasure_media_accept(&state, slots[0], 256, 0, &complete) == FVM_S_CONFLICT);
    puts("PASS checked receiver observations, identity/profile binding and historical repair isolation");
    return 0;
}
