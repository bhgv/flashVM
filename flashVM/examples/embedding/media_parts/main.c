/* Optional bulk-media slots coexist with compact control records; no sockets or VM required. */
#include "flashvm/scontrol.h"    /* Optional tag-zero size negotiation, separate from type binding. */
#include "flashvm/smeasure.h"    /* Actual verified arrivals form bounded deadline feedback. */
#include "flashvm/smedia.h"      /* Public C89 fragment, slot and reassembly contracts. */
#include "limits.h"              /* Bounded tutorial storage and explicitly agreed example tags. */
#include <stdio.h>               /* Report results after all native work has completed. */
#include <string.h>              /* Compare completed bytes and deliberately damage one slot. */

typedef struct ExampleReceiver   /* Caller-owned batch callback policy, no managed references. */
{                                /* Caller-owned batch callback policy, no managed references. */
    FVM_SMediaReceiver receiver; /* Reassembly storage stays alive throughout traversal. */
    size_t damaged;              /* Rejected slot count, including checksum/length failures. */
    int complete;                /* Whole object availability, separate from a playback deadline. */
    int failed;                  /* Unexpected valid-record delivery failure. */
} ExampleReceiver;               /* No hidden allocation or cleanup ownership. */


/* The callback advances only through batch-provided fixed boundaries, never a damaged length. */
static void receive_part(size_t index, FVM_SResult result, const FVM_SRecord *record, void *raw) /* Called synchronously. */
{                                                                                                /* Called synchronously. */
    ExampleReceiver *state = (ExampleReceiver *)raw;                                             /* Borrow stable owner state. */
    (void)index;                                                                                 /* Batch position is not an object identity or part number. */
    if (result != FVM_S_OK)                                                                      /* Condition for this action: Damaged slots cannot provide trusted metadata. */
    {                                                                                            /* Damaged slots cannot provide trusted metadata. */
        ++state->damaged;                                                                        /* Leave the missing part unmarked for later policy. */
        return;                                                                                  /* The batch still visits the next known boundary. */
    } /* End rejected-slot branch. */
    if (!record || record->tag != EXAMPLE_MEDIA_TAG) /* A valid checksum does not select the expected type. */
    {                                                /* Begin conditional branch: A valid checksum does not select the expected type. */
        state->failed = 1;                           /* Defer the tutorial error until synchronous traversal returns. */
    } /* End conditional branch: A valid checksum does not select the expected type. */
} /* End damage-only generic batch observer. */


/* Simulate a reliable tag-zero exchange; real hosts authenticate and queue these compact records. */
static int negotiate(FVM_SMediaProfile *profile)                                                                                                                                                 /* No data slot is active before both commits. */
{                                                                                                                                                                                                /* No data slot is active before both commits. */
    FVM_SControlLimits ceiling = {EXAMPLE_MEDIA_TAG, EXAMPLE_MEDIA_CEILING - FVM_SCONTROL_FRAMING, EXAMPLE_MEDIA_CEILING, EXAMPLE_MEDIA_CEILING - FVM_SCONTROL_FRAMING - FVM_SCONTROL_METADATA}; /* Real local maxima. */
    FVM_SControlLimits wanted = {EXAMPLE_MEDIA_TAG, FVM_SSLOT_BYTES - FVM_SCONTROL_FRAMING, FVM_SSLOT_BYTES, FVM_SMEDIA_PAYLOAD};                                                                /* Tutorial requests the small built-in bulk profile. */
    FVM_SControlExchange sender, receiver;                                                                                                                                                       /* Distinct owners for the two ends of this direction. */
    FVM_SControlMessage offer, ack, decoded;                                                                                                                                                     /* Copied fields, never native struct bytes on wire. */
    unsigned char bytes[128];                                                                                                                                                                    /* Compact service records need no slot-sized buffer. */
    size_t used, consumed;                                                                                                                                                                       /* Each decode consumes exactly one complete service record. */
    if (fvm_scontrol_init(&sender, &ceiling) != FVM_S_OK || fvm_scontrol_init(&receiver, &ceiling) != FVM_S_OK)                                                                                  /* Validate local capacities first. */
    {                                                                                                                                                                                            /* Begin conditional branch: Validate local capacities first. */
        return 0;                                                                                                                                                                                /* No implicit allocation or type binding. */
    } /* End conditional branch: Validate local capacities first. */
    if (fvm_scontrol_propose(&sender, EXAMPLE_NEGOTIATION_ID, &wanted, &offer) != FVM_S_OK || fvm_scontrol_write(&offer, bytes, sizeof(bytes), &used) != FVM_S_OK) /* Pending offer only. */
    {                                                                                                                                                              /* Begin conditional branch: Pending offer only. */
        return 0;                                                                                                                                                  /* Existing payload configuration remains unchanged. */
    } /* End conditional branch: Pending offer only. */
    if (fvm_scontrol_read(bytes, used, &decoded, &consumed) != FVM_S_OK || consumed != used || fvm_scontrol_respond(&receiver, &decoded, &ack) != FVM_S_OK) /* Receive and cap the offer. */
    {                                                                                                                                                       /* Begin conditional branch: Receive and cap the offer. */
        return 0;                                                                                                                                           /* Do not accept uncorrelated or malformed control. */
    } /* End conditional branch: Receive and cap the offer. */
    if (fvm_scontrol_write(&ack, bytes, sizeof(bytes), &used) != FVM_S_OK || fvm_scontrol_commit(&receiver, &ack) != FVM_S_OK) /* Here encoding represents the in-memory ACK queue. */
    {                                                                                                                          /* Begin conditional branch: Here encoding represents the in-memory ACK queue. */
        return 0;                                                                                                              /* A network host commits only after successfully queuing its ACK. */
    } /* End conditional branch: Here encoding represents the in-memory ACK queue. */
    if (fvm_scontrol_read(bytes, used, &decoded, &consumed) != FVM_S_OK || consumed != used || fvm_scontrol_commit(&sender, &decoded) != FVM_S_OK) /* Initiator commits checked ACK. */
    {                                                                                                                                              /* Begin conditional branch: Initiator commits checked ACK. */
        return 0;                                                                                                                                  /* A proposal alone never changes the data framing. */
    } /* End conditional branch: Initiator commits checked ACK. */
    profile->slot_bytes = (size_t)sender.active.slot_bytes;                                     /* Copy agreed size into the typed data adapter. */
    profile->part_bytes = sender.active.part_bytes;                                             /* The peer may have selected a smaller part quota. */
    return profile->slot_bytes == FVM_SSLOT_BYTES && profile->part_bytes == FVM_SMEDIA_PAYLOAD; /* Arrays below intentionally use this requested tutorial geometry. */
} /* End explicit tag-zero exchange. */


/* Encode all parts with exact final length; output belongs to the caller's contiguous batch. */
static int encode(const FVM_SMediaProfile *profile, const FVM_SMediaInfo *info, const unsigned char *source, unsigned char *slots)                                                                                                    /* No allocation. */
{                                                                                                                                                                                                                                     /* No allocation. */
    size_t part, size, used;                                                                                                                                                                                                          /* Native loop counters and checked writer result extent. */
    for (part = 0; part < EXAMPLE_MEDIA_PARTS; ++part)                                                                                                                                                                                /* Pack one S-record per optional bulk slot. */
    {                                                                                                                                                                                                                                 /* Pack one S-record per optional bulk slot. */
        size = part + 1 == EXAMPLE_MEDIA_PARTS ? EXAMPLE_MEDIA_TAIL : FVM_SMEDIA_PAYLOAD;                                                                                                                                             /* Final remainder. */
        if (fvm_smedia_write_profile(profile, EXAMPLE_MEDIA_TAG, info, (unsigned long)part, source + part * FVM_SMEDIA_PAYLOAD, size, slots + part * FVM_SSLOT_BYTES, FVM_SSLOT_BYTES, &used) != FVM_S_OK || used != FVM_SSLOT_BYTES) /* Atomic write. */
        {                                                                                                                                                                                                                             /* Begin conditional branch: Atomic write. */
            return 0;                                                                                                                                                                                                                 /* Do not send an incompletely encoded batch. */
        } /* End conditional branch: Atomic write. */
    } /* Every slot is complete and padded. */
    return 1; /* Caller may batch or transmit individual slots. */
} /* End bounded sender. */


/* No SDK startup: buffers, identities and loss policy are all explicit native owner data. */
int main(void)                                                                          /* Console is only a report around the reusable C API. */
{                                                                                       /* Console is only a report around the reusable C API. */
    unsigned char source[EXAMPLE_MEDIA_BYTES], output[EXAMPLE_MEDIA_BYTES];             /* Caller owns bytes. */
    unsigned char slots[EXAMPLE_MEDIA_PARTS * FVM_SSLOT_BYTES], retry[FVM_SSLOT_BYTES]; /* Bulk transport buffers. */
    unsigned char control[64];                                                          /* Small control message retains variable-length encoding. */
    FVM_SMediaInfo info = {FVM_SMEDIA_FILE, 7, 1, 1, 0, EXAMPLE_MEDIA_BYTES};           /* Authenticated expected identity. */
    FVM_SMediaProfile profile;                                                          /* Explicitly negotiated geometry, not a channel-wide default. */
    FVM_SMeasureMedia observation;                                                      /* Borrows only the native receiver/storage through each closed window. */
    FVM_SAdaptFeedback feedback;                                                        /* Owned historical counts remain unchanged after file repair. */
    ExampleReceiver state;                                                              /* No heap allocation or external subsystem. */
    FVM_SSlotReport report;                                                             /* Independent validity of each fixed boundary. */
    FVM_SMediaPiece piece;                                                              /* Borrowed piece view for immediate media consumers. */
    unsigned long holes[EXAMPLE_MEDIA_PARTS];                                           /* Bounded retransmission request list. */
    size_t i, count, used;                                                              /* Enumeration and encoded extents, never native fields on wire. */
    memset(&state, 0, sizeof(state));                                                   /* Initialize tutorial-only flags before callbacks. */
    for (i = 0; i < sizeof(source); ++i)                                                /* Distinct bytes expose wrong offsets or short tails. */
    {                                                                                   /* Begin loop body: Distinct bytes expose wrong offsets or short tails. */
        source[i] = (unsigned char)(i % 251U);                                          /* Deterministic fixture, no codec dependency. */
    } /* End loop body: Distinct bytes expose wrong offsets or short tails. */
    if (!negotiate(&profile)) /* Agree the optional bulk geometry through compact tag-zero messages. */
    {                         /* Begin conditional branch: Agree the optional bulk geometry through compact tag-zero messages. */
        return 1;             /* No received data packet can negotiate these sizes. */
    } /* End conditional branch: Agree the optional bulk geometry through compact tag-zero messages. */
    if (!encode(&profile, &info, source, slots)) /* Encode three ordinary S-records inside fixed slots. */
    {                                            /* Begin conditional branch: Encode three ordinary S-records inside fixed slots. */
        return 1;                                /* An encoding failure produces no successful example report. */
    } /* End conditional branch: Encode three ordinary S-records inside fixed slots. */
    memcpy(retry, slots + FVM_SSLOT_BYTES, sizeof(retry));                                                                         /* Sender retains just the missing part here. */
    memset(slots + FVM_SSLOT_BYTES, 0x7f, FVM_SSLOT_BYTES);                                                                        /* Destroy checksum, length and body. */
    if (fvm_smedia_receiver_init_profile(&state.receiver, &profile, EXAMPLE_MEDIA_TAG, &info, output, sizeof(output)) != FVM_S_OK) /* No output bytes touched. */
    {                                                                                                                              /* Begin conditional branch: No output bytes touched. */
        return 1;                                                                                                                  /* Host refuses an invalid buffer/identity before network input. */
    } /* End conditional branch: No output bytes touched. */
    if (fvm_smeasure_media_init(&observation, &state.receiver, info.object, EXAMPLE_MEASURE_DEADLINE) != FVM_S_OK) /* Condition for this action: Bind pristine agreed identity/profile. */
    {                                                                                                              /* Begin conditional branch: Condition for this action: Bind pristine agreed identity/profile. */
        return 1;                                                                                                  /* Bind pristine agreed identity/profile. */
    } /* End conditional branch: Condition for this action: Bind pristine agreed identity/profile. */
    if (fvm_sslot_batch(slots, sizeof(slots), receive_part, &state, &report) != FVM_S_OK || report.valid != 2 || state.damaged != 1 || state.failed) /* Fixed resynchronization. */
    {                                                                                                                                                /* Begin conditional branch: Fixed resynchronization. */
        return 1;                                                                                                                                    /* The third slot must remain independently readable. */
    } /* End conditional branch: Fixed resynchronization. */
    for (i = 0; i < EXAMPLE_MEDIA_PARTS; ++i)                                                                                                          /* A host may route each known slot by its trusted profile. */
    {                                                                                                                                                  /* A host may route each known slot by its trusted profile. */
        FVM_SResult result = fvm_smeasure_media_accept(&observation, slots + i * FVM_SSLOT_BYTES, FVM_SSLOT_BYTES, (unsigned long)i, &state.complete); /* Verify before copy. */
        if ((i == 1 && result == FVM_S_OK) || (i != 1 && result != FVM_S_OK))                                                                          /* Only the deliberately damaged part fails. */
        {                                                                                                                                              /* Begin conditional branch: Only the deliberately damaged part fails. */
            return 1;                                                                                                                                  /* Never accept bytes from an unchecked record. */
        } /* End conditional branch: Only the deliberately damaged part fails. */
    } /* Valid first and final parts now coexist around an unfilled hole. */
    if (fvm_smeasure_media_finish(&observation, EXAMPLE_MEASURE_DEADLINE, 1, EXAMPLE_MEASURE_RTT, &feedback) != FVM_S_OK || feedback.on_time != 2 || feedback.late || feedback.damaged) /* Condition for this action: Corrupt unidentified slot stays missing. */
    {                                                                                                                                                                                   /* Begin conditional branch: Condition for this action: Corrupt unidentified slot stays missing. */
        return 1;                                                                                                                                                                       /* Corrupt unidentified slot stays missing. */
    } /* End conditional branch: Condition for this action: Corrupt unidentified slot stays missing. */
    if (fvm_sadapt_feedback_write(&feedback, control, sizeof(control), &used) != FVM_S_OK) /* Condition for this action: Compact service-3 report, without bulk padding. */
    {                                                                                      /* Begin conditional branch: Condition for this action: Compact service-3 report, without bulk padding. */
        return 1;                                                                          /* Compact service-3 report, without bulk padding. */
    } /* End conditional branch: Condition for this action: Compact service-3 report, without bulk padding. */
    if (fvm_smedia_missing(&state.receiver, 0, holes, EXAMPLE_MEDIA_PARTS, &count) != FVM_S_OK || count != 1 || holes[0] != 1) /* File policy. */
    {                                                                                                                          /* Begin conditional branch: File policy. */
        return 1;                                                                                                              /* Request exactly part one through the host's reliable control channel. */
    } /* End conditional branch: File policy. */
    if (fvm_smedia_accept(&state.receiver, retry, sizeof(retry), &state.complete) != FVM_S_OK || !state.complete || memcmp(source, output, sizeof(source))) /* Only one part retried. */
    {                                                                                                                                                       /* Begin conditional branch: Only one part retried. */
        return 1;                                                                                                                                           /* Completion means every exact object byte was copied. */
    } /* End conditional branch: Only one part retried. */
    if (feedback.on_time != 2 || observation.measurement.report.on_time != 2) /* Condition for this action: Late FILE repair cannot improve frozen history. */
    {                                                                         /* Begin conditional branch: Condition for this action: Late FILE repair cannot improve frozen history. */
        return 1;                                                             /* Late FILE repair cannot improve frozen history. */
    } /* End conditional branch: Condition for this action: Late FILE repair cannot improve frozen history. */
    info.mode = FVM_SMEDIA_LIVE;                                                                                                                                              /* Reuse storage for a separately agreed immediate-media object. */
    ++info.object;                                                                                                                                                            /* Do not reuse the previous object's wire identity. */
    info.time_ms = 40;                                                                                                                                                        /* Host owns presentation clock/deadline interpretation. */
    if (!encode(&profile, &info, source, slots) || fvm_smedia_receiver_init_profile(&state.receiver, &profile, EXAMPLE_MEDIA_TAG, &info, output, sizeof(output)) != FVM_S_OK) /* Abandon prior state explicitly. */
    {                                                                                                                                                                         /* Begin conditional branch: Abandon prior state explicitly. */
        return 1;                                                                                                                                                             /* No automatic object adoption from incoming packets. */
    } /* End conditional branch: Abandon prior state explicitly. */
    if (fvm_smedia_read_profile(&profile, slots, FVM_SSLOT_BYTES, EXAMPLE_MEDIA_TAG, &piece) != FVM_S_OK || piece.part != 0 || piece.size != FVM_SMEDIA_PAYLOAD) /* Immediate piece consumer. */
    {                                                                                                                                                            /* Begin conditional branch: Immediate piece consumer. */
        return 1;                                                                                                                                                /* A real codec consumes the verified borrowed bytes before reuse. */
    } /* End conditional branch: Immediate piece consumer. */
    if (fvm_smeasure_media_init(&observation, &state.receiver, info.object, EXAMPLE_MEASURE_DEADLINE) != FVM_S_OK) /* Condition for this action: New independent live observation. */
    {                                                                                                              /* Begin conditional branch: Condition for this action: New independent live observation. */
        return 1;                                                                                                  /* New independent live observation. */
    } /* End conditional branch: Condition for this action: New independent live observation. */
    if (fvm_smeasure_media_accept(&observation, slots, FVM_SSLOT_BYTES, 0, &state.complete) != FVM_S_OK || state.complete) /* Other parts intentionally omitted. */
    {                                                                                                                      /* Begin conditional branch: Other parts intentionally omitted. */
        return 1;                                                                                                          /* The host may discard missing parts when its deadline expires. */
    } /* End conditional branch: Other parts intentionally omitted. */
    if (fvm_smedia_missing(&state.receiver, 0, holes, EXAMPLE_MEDIA_PARTS, &count) != FVM_S_OK || count) /* Live policy suppresses retry requests. */
    {                                                                                                    /* Begin conditional branch: Live policy suppresses retry requests. */
        return 1;                                                                                        /* No automatic retransmission delays immediate playback. */
    } /* End conditional branch: Live policy suppresses retry requests. */
    if (fvm_smeasure_media_finish(&observation, EXAMPLE_MEASURE_DEADLINE, 0, 0, &feedback) != FVM_S_OK || feedback.on_time != 1 || feedback.rtt_known) /* Condition for this action: Unknown RTT remains explicit, never guessed. */
    {                                                                                                                                                  /* Begin conditional branch: Condition for this action: Unknown RTT remains explicit, never guessed. */
        return 1;                                                                                                                                      /* Unknown RTT remains explicit, never guessed. */
    } /* End conditional branch: Condition for this action: Unknown RTT remains explicit, never guessed. */
    if (fvm_srecord_write(EXAMPLE_CONTROL_TAG, "ok", 2, control, sizeof(control), &used) != FVM_S_OK || used >= FVM_SSLOT_BYTES) /* Small messages stay compact. */
    {                                                                                                                            /* Begin conditional branch: Small messages stay compact. */
        return 1;                                                                                                                /* No slot padding is required for ordinary control records. */
    } /* End conditional branch: Small messages stay compact. */
    printf("PASS compact control, bulk batch recovery, file retry and live no-retry policy\n"); /* Stable tutorial result. */
    return 0;                                                                                   /* Caller-owned buffers need no library cleanup. */
} /* End tutorial. */
