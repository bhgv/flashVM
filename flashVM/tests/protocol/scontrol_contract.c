/* Native C89 control/slot/fragment lifetime contract, suitable for sanitizers and cross-linking. */
#include "flashvm/scontrol.h"
#include "flashvm/smedia.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "media-control:%d %s\n", __LINE__, #x);                                \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
/* No protocol value below is serialized by storing a C structure. */
int main(void)
{
    FVM_SControlLimits ceiling = {19, 65504, 65536, 65440};
    FVM_SControlLimits local = {19, 4064, 4096, 4000};
    FVM_SControlExchange sender, receiver, before;
    FVM_SControlMessage offer, ack, decoded;
    FVM_SMediaProfile profile;
    FVM_SMediaInfo info = {FVM_SMEDIA_FILE, 7, 8, 9, 40, 8017};
    FVM_SMediaReceiver media;
    FVM_SMediaPiece piece;
    FVM_SRecord record;
    unsigned char control[128], *slot, *source, *destination;
    size_t used, i, count;
    unsigned long holes[4], part, phase;
    FVM_SControlBoundary boundary = {7, 8, 10};
    int complete;
    CHECK(fvm_scontrol_init(&sender, &ceiling) == FVM_S_OK);
    CHECK(fvm_scontrol_init(&receiver, &local) == FVM_S_OK);
    CHECK(fvm_scontrol_propose(&sender, 1, &ceiling, &offer) == FVM_S_OK && !sender.active_id);
    CHECK(fvm_scontrol_write(&offer, control, sizeof(control), &used) == FVM_S_OK &&
          used < sizeof(control));
    CHECK(fvm_scontrol_read(control, used, &decoded, &count) == FVM_S_OK && count == used);
    CHECK(fvm_scontrol_respond(&receiver, &decoded, &ack) == FVM_S_OK && !receiver.active_id);
    CHECK(ack.limits.slot_bytes == 4096 && ack.limits.part_bytes == 4000);
    CHECK(fvm_scontrol_commit(&receiver, &ack) == FVM_S_OK);
    CHECK(fvm_scontrol_write(&ack, control, sizeof(control), &used) == FVM_S_OK);
    CHECK(fvm_scontrol_read(control, used, &decoded, &count) == FVM_S_OK);
    CHECK(fvm_scontrol_commit(&sender, &decoded) == FVM_S_OK && sender.active_id == 1);
    CHECK(fvm_scontrol_commit(&sender, &decoded) == FVM_S_OK);
    before = sender;
    decoded.limits.tag = 20;
    CHECK(fvm_scontrol_commit(&sender, &decoded) == FVM_S_CONFLICT &&
          !memcmp(&before, &sender, sizeof(sender)));
    CHECK(fvm_scontrol_propose(&sender, 2, &local, &offer) == FVM_S_OK);
    CHECK(fvm_scontrol_cancel(&sender) == FVM_S_OK && sender.active_id == 1);
    CHECK(fvm_scontrol_propose(&sender, 2, &local, &offer) == FVM_S_CONFLICT);
    profile.slot_bytes = (size_t)sender.active.slot_bytes;
    profile.part_bytes = sender.active.part_bytes;
    slot = (unsigned char *)malloc((size_t)FVM_SSLOT_MAX_BYTES);
    source = (unsigned char *)malloc((size_t)FVM_SSLOT_MAX_BYTES);
    destination = (unsigned char *)malloc((size_t)info.total);
    CHECK(slot && source && destination);
    for (i = 0; i < (size_t)FVM_SSLOT_MAX_BYTES; ++i)
        source[i] = (unsigned char)(i % 251U);
    CHECK(fvm_smedia_receiver_init_profile(&media, &profile, 19, &info, destination,
                                           (size_t)info.total) == FVM_S_OK);
    for (part = 0; part < 3; ++part)
    {
        count = part == 2 ? 17 : 4000;
        CHECK(fvm_smedia_write_profile(&profile, 19, &info, part, source + part * 4000, count, slot,
                                       profile.slot_bytes, &used) == FVM_S_OK &&
              used == profile.slot_bytes);
        CHECK(fvm_smedia_read_profile(&profile, slot, used, 19, &piece) == FVM_S_OK &&
              piece.part == part);
        CHECK(fvm_smedia_accept(&media, slot, used, &complete) == FVM_S_OK);
        CHECK(fvm_smedia_accept(&media, slot, used, &complete) == FVM_S_OK &&
              media.received == part + 1);
    }
    CHECK(complete && !memcmp(source, destination, (size_t)info.total));
    CHECK(fvm_smedia_missing(&media, 0, holes, 4, &count) == FVM_S_OK && !count);
    /* Large agreed slots do not require a slot-sized local stack frame in the library. */
    CHECK(fvm_sslot_write_sized(19, source, 60000, (size_t)FVM_SSLOT_MAX_BYTES, slot,
                                (size_t)FVM_SSLOT_MAX_BYTES, &used) == FVM_S_OK);
    CHECK(fvm_sslot_read_sized(slot, used, used, &record, &count) == FVM_S_OK &&
          record.size == 60000);
    CHECK(!memcmp(record.payload, source, record.size));
    /* Compact mode has no padding and remains a distinct agreed state. */
    local.record_limit = 63;
    local.slot_bytes = local.part_bytes = 0;
    CHECK(fvm_scontrol_propose(&sender, 3, &local, &offer) == FVM_S_OK);
    CHECK(fvm_scontrol_respond(&receiver, &offer, &ack) == FVM_S_OK);
    CHECK(fvm_scontrol_commit(&receiver, &ack) == FVM_S_OK);
    CHECK(fvm_scontrol_commit(&sender, &ack) == FVM_S_OK && !sender.active.slot_bytes);
    CHECK(fvm_srecord_write(19, "ok", 2, control, sizeof(control), &used) == FVM_S_OK && used < 16);
    /* Change bulk geometry at future objects in both directions without stopping a timeline. */
    info.mode = FVM_SMEDIA_LIVE;
    for (phase = 0; phase < 3; ++phase)
    {
        local.slot_bytes = phase == 1 ? 512 : 4096;
        local.record_limit = local.slot_bytes - FVM_SCONTROL_FRAMING;
        local.part_bytes = phase == 1 ? 400 : 4000;
        boundary.object = 10 + phase;
        CHECK(fvm_scontrol_propose_at(&sender, 4 + phase, &local, &boundary, &offer) == FVM_S_OK);
        CHECK(fvm_scontrol_write(&offer, control, sizeof(control), &used) == FVM_S_OK);
        CHECK(fvm_scontrol_read(control, used, &decoded, &count) == FVM_S_OK);
        CHECK(fvm_scontrol_respond(&receiver, &decoded, &ack) == FVM_S_OK);
        CHECK(fvm_scontrol_commit(&receiver, &ack) == FVM_S_OK);
        CHECK(fvm_scontrol_commit(&sender, &ack) == FVM_S_OK);
        info.object = boundary.object;
        CHECK(fvm_smedia_receiver_init_agreed(&media, &sender, &info, destination,
                                              (size_t)info.total) == FVM_S_OK);
        --info.object;
        CHECK(fvm_smedia_receiver_init_agreed(&media, &sender, &info, destination,
                                              (size_t)info.total) == FVM_S_CONFLICT);
        ++info.object;
        profile.slot_bytes = (size_t)sender.active.slot_bytes;
        profile.part_bytes = sender.active.part_bytes;
        for (part = 0; part < (info.total - 1) / profile.part_bytes + 1; ++part)
        {
            count = (size_t)(info.total - part * profile.part_bytes);
            if (count > profile.part_bytes)
                count = (size_t)profile.part_bytes;
            CHECK(fvm_smedia_write_profile(&profile, 19, &info, part,
                                           source + part * profile.part_bytes, count, slot,
                                           profile.slot_bytes, &used) == FVM_S_OK);
            CHECK(fvm_smedia_accept(&media, slot, used, &complete) == FVM_S_OK);
        }
        CHECK(complete && !memcmp(source, destination, (size_t)info.total));
    }
    free(destination);
    free(source);
    free(slot);
    puts("PASS negotiated limits, exact commit, large slots, configured media and compact records");
    return 0;
}
