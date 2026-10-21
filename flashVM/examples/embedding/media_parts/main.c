/* Optional bulk-media slots coexist with compact control records; no sockets or VM required. */
#include "flashvm/smedia.h" /* Public C89 fragment, slot and reassembly contracts. */
#include "limits.h" /* Bounded tutorial storage and explicitly agreed example tags. */
#include <stdio.h> /* Report results after all native work has completed. */
#include <string.h> /* Compare completed bytes and deliberately damage one slot. */
typedef struct ExampleReceiver { /* Caller-owned batch callback policy, no managed references. */
    FVM_SMediaReceiver receiver; /* Reassembly storage stays alive throughout traversal. */
    size_t damaged; /* Rejected slot count, including checksum/length failures. */
    int complete; /* Whole object availability, separate from a playback deadline. */
    int failed; /* Unexpected valid-record delivery failure. */
} ExampleReceiver; /* No hidden allocation or cleanup ownership. */
/* The callback advances only through batch-provided fixed boundaries, never a damaged length. */
static void receive_part(size_t index, FVM_SResult result, const FVM_SRecord *record, void *raw) { /* Called synchronously. */
    ExampleReceiver *state = (ExampleReceiver *)raw; /* Borrow stable owner state. */
    (void)index; /* Batch position is not an object identity or part number. */
    if (result != FVM_S_OK) { /* Damaged slots cannot provide trusted metadata. */
        ++state->damaged; /* Leave the missing part unmarked for later policy. */
        return; /* The batch still visits the next known boundary. */
    } /* End rejected-slot branch. */
    if (!record || record->tag != EXAMPLE_MEDIA_TAG) /* A valid checksum does not select the expected type. */
        state->failed = 1; /* Defer the tutorial error until synchronous traversal returns. */
} /* End damage-only generic batch observer. */
/* Encode all parts with exact final length; output belongs to the caller's contiguous batch. */
static int encode(const FVM_SMediaInfo *info, const unsigned char *source, unsigned char *slots) { /* No allocation. */
    size_t part, size, used; /* Native loop counters and checked writer result extent. */
    for (part = 0; part < EXAMPLE_MEDIA_PARTS; ++part) { /* Pack one S-record per optional bulk slot. */
        size = part + 1 == EXAMPLE_MEDIA_PARTS ? EXAMPLE_MEDIA_TAIL : FVM_SMEDIA_PAYLOAD; /* Final remainder. */
        if (fvm_smedia_write(EXAMPLE_MEDIA_TAG, info, (unsigned long)part, source + part * FVM_SMEDIA_PAYLOAD, size, slots + part * FVM_SSLOT_BYTES, FVM_SSLOT_BYTES, &used) != FVM_S_OK || used != FVM_SSLOT_BYTES) /* Atomic write. */
            return 0; /* Do not send an incompletely encoded batch. */
    } /* Every slot is complete and padded. */
    return 1; /* Caller may batch or transmit individual slots. */
} /* End bounded sender. */
/* No SDK startup: buffers, identities and loss policy are all explicit native owner data. */
int main(void) { /* Console is only a report around the reusable C API. */
    unsigned char source[EXAMPLE_MEDIA_BYTES], output[EXAMPLE_MEDIA_BYTES]; /* Caller owns bytes. */
    unsigned char slots[EXAMPLE_MEDIA_PARTS * FVM_SSLOT_BYTES], retry[FVM_SSLOT_BYTES]; /* Bulk transport buffers. */
    unsigned char control[64]; /* Small control message retains variable-length encoding. */
    FVM_SMediaInfo info = {FVM_SMEDIA_FILE, 7, 1, 1, 0, EXAMPLE_MEDIA_BYTES}; /* Authenticated expected identity. */
    ExampleReceiver state; /* No heap allocation or external subsystem. */
    FVM_SSlotReport report; /* Independent validity of each fixed boundary. */
    FVM_SMediaPiece piece; /* Borrowed piece view for immediate media consumers. */
    unsigned long holes[EXAMPLE_MEDIA_PARTS]; /* Bounded retransmission request list. */
    size_t i, count, used; /* Enumeration and encoded extents, never native fields on wire. */
    memset(&state, 0, sizeof(state)); /* Initialize tutorial-only flags before callbacks. */
    for (i = 0; i < sizeof(source); ++i) /* Distinct bytes expose wrong offsets or short tails. */
        source[i] = (unsigned char)(i % 251U); /* Deterministic fixture, no codec dependency. */
    if (!encode(&info, source, slots)) /* Encode three ordinary S-records inside fixed slots. */
        return 1; /* An encoding failure produces no successful example report. */
    memcpy(retry, slots + FVM_SSLOT_BYTES, sizeof(retry)); /* Sender retains just the missing part here. */
    memset(slots + FVM_SSLOT_BYTES, 0x7f, FVM_SSLOT_BYTES); /* Destroy checksum, length and body. */
    if (fvm_smedia_receiver_init(&state.receiver, EXAMPLE_MEDIA_TAG, &info, output, sizeof(output)) != FVM_S_OK) /* No output bytes touched. */
        return 1; /* Host refuses an invalid buffer/identity before network input. */
    if (fvm_sslot_batch(slots, sizeof(slots), receive_part, &state, &report) != FVM_S_OK || report.valid != 2 || state.damaged != 1 || state.failed) /* Fixed resynchronization. */
        return 1; /* The third slot must remain independently readable. */
    for (i = 0; i < EXAMPLE_MEDIA_PARTS; ++i) { /* A host may route each known slot by its trusted profile. */
        FVM_SResult result = fvm_smedia_accept(&state.receiver, slots + i * FVM_SSLOT_BYTES, FVM_SSLOT_BYTES, &state.complete); /* Verify before copy. */
        if ((i == 1 && result == FVM_S_OK) || (i != 1 && result != FVM_S_OK)) /* Only the deliberately damaged part fails. */
            return 1; /* Never accept bytes from an unchecked record. */
    } /* Valid first and final parts now coexist around an unfilled hole. */
    if (fvm_smedia_missing(&state.receiver, 0, holes, EXAMPLE_MEDIA_PARTS, &count) != FVM_S_OK || count != 1 || holes[0] != 1) /* File policy. */
        return 1; /* Request exactly part one through the host's reliable control channel. */
    if (fvm_smedia_accept(&state.receiver, retry, sizeof(retry), &state.complete) != FVM_S_OK || !state.complete || memcmp(source, output, sizeof(source))) /* Only one part retried. */
        return 1; /* Completion means every exact object byte was copied. */
    info.mode = FVM_SMEDIA_LIVE; /* Reuse storage for a separately agreed immediate-media object. */
    ++info.object; /* Do not reuse the previous object's wire identity. */
    info.time_ms = 40; /* Host owns presentation clock/deadline interpretation. */
    if (!encode(&info, source, slots) || fvm_smedia_receiver_init(&state.receiver, EXAMPLE_MEDIA_TAG, &info, output, sizeof(output)) != FVM_S_OK) /* Abandon prior state explicitly. */
        return 1; /* No automatic object adoption from incoming packets. */
    if (fvm_smedia_read(slots, FVM_SSLOT_BYTES, EXAMPLE_MEDIA_TAG, &piece) != FVM_S_OK || piece.part != 0 || piece.size != FVM_SMEDIA_PAYLOAD) /* Immediate piece consumer. */
        return 1; /* A real codec consumes the verified borrowed bytes before reuse. */
    if (fvm_smedia_accept(&state.receiver, slots, FVM_SSLOT_BYTES, &state.complete) != FVM_S_OK || state.complete) /* Other parts intentionally omitted. */
        return 1; /* The host may discard missing parts when its deadline expires. */
    if (fvm_smedia_missing(&state.receiver, 0, holes, EXAMPLE_MEDIA_PARTS, &count) != FVM_S_OK || count) /* Live policy suppresses retry requests. */
        return 1; /* No automatic retransmission delays immediate playback. */
    if (fvm_srecord_write(EXAMPLE_CONTROL_TAG, "ok", 2, control, sizeof(control), &used) != FVM_S_OK || used >= FVM_SSLOT_BYTES) /* Small messages stay compact. */
        return 1; /* No slot padding is required for ordinary control records. */
    printf("PASS compact control, bulk batch recovery, file retry and live no-retry policy\n"); /* Stable tutorial result. */
    return 0; /* Caller-owned buffers need no library cleanup. */
} /* End tutorial. */
