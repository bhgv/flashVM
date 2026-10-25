/* Real linked SWF frames -> immutable leases -> negotiated S-record parts in a host-owned file. */
#include "../../../frontend/common/player.h"           /* Public synchronous player/SDK boundary. */
#include "../../../frontend/common/resource_frame.h"   /* Native cache and authorized part writer. */
#include "flashvm/smedia.h"                            /* Typed receiving views and FILE/LIVE protocol constants. */
#include "flashvm/sbindings.h"                         /* Separate direction-local tag-zero type declarations. */
#include "flashvm/sadapt.h"                            /* Allocation-free policy; host supplies trusted measurement windows. */
#include "limits.h"                                    /* Explicit tutorial geometry, identities and profile choices. */
#include "../frame_capture/movie.inc"                  /* Same documented deterministic linked SWF8 fixture. */
#include <stdio.h>                                     /* Replaceable file adapter; the library itself performs no output I/O. */
#include <string.h>                                    /* Initialize native state and parse this executable's single path argument. */

typedef struct Export                                  /* Native state is owned by main and borrowed only synchronously. */
{                                                      /* Native state is owned by main and borrowed only synchronously. */
    FVM_RDispatch *dispatch;                           /* Owned private dispatcher, registering only pure cache handlers. */
    FVM_DesktopResourceFrame *frames;                  /* Owned three-slot immutable image cache. */
    FVM_RHandle target;                                /* Exact resource registration incarnation. */
    FVM_SControlExchange sender, receiver;             /* Independent endpoints for one negotiated direction. */
    FVM_DesktopFrameLease held;                        /* Final image retained through runner and SDK shutdown. */
    FVM_SAdapt adaptation;                             /* Stable address: its copied configuration references its internal ladder. */
    FVM_SAdaptFeedback reports[EXAMPLE_FRAME_REPORTS]; /* Three decoded host windows, never invented by the player. */
    FVM_SControlLimits initial_profile;                /* Already selected first-frame geometry. */
    unsigned long goal, previous_parts;                /* Frame limit and causal expected count of the preceding image. */
    int adaptive;                                      /* Replay explicit feedback instead of the original scripted profile sequence. */
    FILE *file;                                        /* Host output adapter, closed only by main. */
    unsigned long mode;                                /* FILE retries versus LIVE omission, independent of the file adapter. */
    unsigned long requests, count, first_hash;         /* Monotonic local IDs, presentations and test witness. */
} Export;                                              /* No borrowed VM/root/source/pixel pointer survives any callback. */


/* Deterministic comparison witness for a repeated encoded slot, not cryptographic integrity. */
static unsigned long hash_slot(const unsigned char *bytes, size_t size) /* No native layout on wire. */
{                                                                       /* No native layout on wire. */
    unsigned long hash = 0;                                             /* Start the same witness for both independent encodings. */
    size_t i;                                                           /* Caller capacity has already bounded the byte extent. */
    for (i = 0; i < size; ++i)                                          /* Mask LP64 equally. */
    {                                                                   /* Begin loop body: Mask LP64 equally. */
        hash = ((hash * 33UL) ^ bytes[i]) & FVM_SRECORD_U32;            /* Mask LP64 equally. */
    } /* End loop body: Mask LP64 equally. */
    return hash; /* Only used to check repeated output after SDK teardown. */
} /* End local witness. */


/* Pump only this private dispatcher's cache operations, never arbitrary live VM handlers. */
static int call(Export *state, unsigned long operation, const void *data, size_t size, FVM_RReply *reply) /* Owner-only. */
{                                                                                                         /* Owner-only. */
    FVM_RRequest request;                                                                                 /* Dispatcher copies metadata and payload before return. */
    size_t done;                                                                                          /* Require exactly the submitted local job to complete. */
    memset(&request, 0, sizeof(request));                                                                 /* No uninitialized transport/session fields. */
    request.session = EXAMPLE_FRAME_SESSION;                                                              /* Trusted native identity. */
    request.id = ++state->requests;                                                                       /* Trusted native identity. */
    request.target = state->target;                                                                       /* Immutable resource incarnation. */
    request.operation = operation;                                                                        /* Immutable resource incarnation. */
    request.data = data;                                                                                  /* Borrowed only through synchronous post. */
    request.size = size;                                                                                  /* Borrowed only through synchronous post. */
    if (fvm_rdispatch_post(state->dispatch, &request) != FVM_R_OK)                                        /* Condition for this action: No accepted job to clean. */
    {                                                                                                     /* Begin conditional branch: Condition for this action: No accepted job to clean. */
        return 0;                                                                                         /* No accepted job to clean. */
    } /* End conditional branch: Condition for this action: No accepted job to clean. */
    if (fvm_rdispatch_pump(state->dispatch, 1, &done) != FVM_R_OK || done != 1)  /* Condition for this action: No other handler shares this queue. */
    {                                                                            /* No other handler shares this queue. */
        fvm_rdispatch_cancel(state->dispatch, request.session, request.id);      /* Cancel a retained queued job. */
        fvm_rdispatch_take(state->dispatch, request.session, request.id, reply); /* Acknowledge cancellation if available. */
        return 0;                                                                /* The callback cannot silently continue with unknown ownership. */
    } /* Pure native handlers have returned before taking their result. */
    return fvm_rdispatch_take(state->dispatch, request.session, request.id, reply) == FVM_R_OK && reply->result == FVM_R_OK; /* Copied result. */
} /* End native local operation adapter. */


/* Encode a canonical release request; no implicit lease lifetime is tied to closing a file. */
static int release(Export *state, FVM_DesktopFrameLease lease)                 /* The supplied lease belongs to this session. */
{                                                                              /* The supplied lease belongs to this session. */
    FVM_SInteger number;                                                       /* Portable signed-magnitude integer, not native-endian stores. */
    unsigned char bytes[2 * FVM_SRECORD_INTEGER_MAX];                          /* Two bounded canonical fields. */
    size_t first, second;                                                      /* Exact extents returned by the public integer writer. */
    FVM_RReply reply;                                                          /* Release succeeds with an empty copied response. */
    number.high = 0;                                                           /* Nonnegative one-based lease ID. */
    number.negative = 0;                                                       /* Nonnegative one-based lease ID. */
    number.low = lease.id;                                                     /* Nonnegative one-based lease ID. */
    if (fvm_sinteger_write(&number, bytes, sizeof(bytes), &first) != FVM_S_OK) /* Condition for this action: Validate first field. */
    {                                                                          /* Begin conditional branch: Condition for this action: Validate first field. */
        return 0;                                                              /* Validate first field. */
    } /* End conditional branch: Condition for this action: Validate first field. */
    number.low = lease.generation;                                                              /* Never reuse a retired generation. */
    if (fvm_sinteger_write(&number, bytes + first, sizeof(bytes) - first, &second) != FVM_S_OK) /* Condition for this action: Second field. */
    {                                                                                           /* Begin conditional branch: Condition for this action: Second field. */
        return 0;                                                                               /* Second field. */
    } /* End conditional branch: Condition for this action: Second field. */
    return call(state, FVM_DESKTOP_RFRAME_RELEASE, bytes, first + second, &reply) && !reply.size; /* Explicit release. */
} /* End pin release. */


/* The file adapter stores ordinary compact service records, then explicit fixed-size data slots. */
static int service(Export *state, const FVM_SControlMessage *message, FVM_SControlMessage *decoded) /* Simulated reliable control transport. */
{                                                                                                   /* Simulated reliable control transport. */
    unsigned char bytes[128];                                                                       /* Both size proposals and ACKs remain small variable-length records. */
    size_t used, consumed;                                                                          /* Decode exactly the record successfully queued to this file. */
    return fvm_scontrol_write(message, bytes, sizeof(bytes), &used) == FVM_S_OK &&                  /* No padding. */
           fwrite(bytes, 1, used, state->file) == used &&                                           /* Replace this adapter with a reliable authenticated channel. */
           fvm_scontrol_read(bytes, used, decoded, &consumed) == FVM_S_OK && consumed == used;      /* Checked copied metadata. */
} /* End small control adapter. */


/* Decode the complete bounded host trace before opening output or starting any VM. */
static int feedback(Export *state, const char *path)                             /* Trace is trusted local input, not authenticated network traffic. */
{                                                                                /* Trace is trusted local input, not authenticated network traffic. */
    unsigned char bytes[EXAMPLE_FRAME_REPORT_BYTES * EXAMPLE_FRAME_REPORTS + 1]; /* One extra byte detects oversize input. */
    FILE *file = fopen(path, "rb");                                              /* Borrow path only while opening the input adapter. */
    FVM_SAdaptConfig config;                                                     /* Defaults are explicitly tuned for this short four-frame demonstration. */
    FVM_SControlLimits profiles[3];                                              /* Increasing supported geometries copied by policy initialization. */
    size_t size, offset = 0, used, i;                                            /* No unbounded allocation or transport reads. */
    int valid;                                                                   /* No unbounded allocation or transport reads. */
    if (!file)                                                                   /* Condition for this action: Output remains untouched on input-open failure. */
    {                                                                            /* Begin conditional branch: Condition for this action: Output remains untouched on input-open failure. */
        return 0;                                                                /* Output remains untouched on input-open failure. */
    } /* End conditional branch: Condition for this action: Output remains untouched on input-open failure. */
    size = fread(bytes, 1, sizeof(bytes), file); /* Bounded complete trace read. */
    valid = !ferror(file);                       /* Bounded complete trace read. */
    if (fclose(file))                            /* Condition for this action: Always close input before any output is opened. */
    {                                            /* Begin conditional branch: Condition for this action: Always close input before any output is opened. */
        valid = 0;                               /* Always close input before any output is opened. */
    } /* End conditional branch: Condition for this action: Always close input before any output is opened. */
    if (!valid || size == sizeof(bytes)) /* Condition for this action: Reject I/O failure and oversize traces. */
    {                                    /* Begin conditional branch: Condition for this action: Reject I/O failure and oversize traces. */
        return 0;                        /* Reject I/O failure and oversize traces. */
    } /* End conditional branch: Condition for this action: Reject I/O failure and oversize traces. */
    for (i = 0; i < EXAMPLE_FRAME_REPORTS; ++i)                                                             /* Exactly three compact service reports. */
    {                                                                                                       /* Exactly three compact service reports. */
        if (fvm_sadapt_feedback_read(bytes + offset, size - offset, &state->reports[i], &used) != FVM_S_OK) /* Condition for this action: CRC and count validation. */
        {                                                                                                   /* Begin conditional branch: Condition for this action: CRC and count validation. */
            return 0;                                                                                       /* CRC and count validation. */
        } /* End conditional branch: Condition for this action: CRC and count validation. */
        if (state->reports[i].stream != EXAMPLE_FRAME_STREAM || state->reports[i].epoch != EXAMPLE_FRAME_EPOCH || state->reports[i].window != i + 1) /* Condition for this action: Reject unrelated or reordered windows. */
        {                                                                                                                                            /* Begin conditional branch: Condition for this action: Reject unrelated or reordered windows. */
            return 0;                                                                                                                                /* Reject unrelated or reordered windows. */
        } /* End conditional branch: Condition for this action: Reject unrelated or reordered windows. */
        offset += used; /* Advance only by a fully verified canonical record. */
    } /* No speculative policy changes while validating input. */
    if (offset != size || fvm_sadapt_config_init(&config) != FVM_S_OK) /* Condition for this action: Trailing bytes are an input error. */
    {                                                                  /* Begin conditional branch: Condition for this action: Trailing bytes are an input error. */
        return 0;                                                      /* Trailing bytes are an input error. */
    } /* End conditional branch: Condition for this action: Trailing bytes are an input error. */
    profiles[0].slot_bytes = EXAMPLE_FRAME_SLOT_SMALL;                            /* Smaller bulk chunks. */
    profiles[0].part_bytes = EXAMPLE_FRAME_PART_SMALL;                            /* Smaller bulk chunks. */
    profiles[1].slot_bytes = EXAMPLE_FRAME_SLOT_NORMAL;                           /* Initial geometry. */
    profiles[1].part_bytes = EXAMPLE_FRAME_PART_NORMAL;                           /* Initial geometry. */
    profiles[2].slot_bytes = EXAMPLE_FRAME_SLOT_LARGE;                            /* Larger bulk chunks. */
    profiles[2].part_bytes = EXAMPLE_FRAME_PART_LARGE;                            /* Larger bulk chunks. */
    for (i = 0; i < 3; ++i)                                                       /* All ladder entries share the declared type and real framing reserve. */
    {                                                                             /* All ladder entries share the declared type and real framing reserve. */
        profiles[i].tag = EXAMPLE_FRAME_TAG;                                      /* Distinguish raw data from encoded extent. */
        profiles[i].record_limit = profiles[i].slot_bytes - FVM_SCONTROL_FRAMING; /* Distinguish raw data from encoded extent. */
    } /* The policy copies this temporary ladder before return. */
    config.profiles = profiles;                                      /* Begin with the already chosen normal profile. */
    config.count = 3;                                                /* Begin with the already chosen normal profile. */
    config.initial = 1;                                              /* Begin with the already chosen normal profile. */
    config.stream = EXAMPLE_FRAME_STREAM;                            /* Direction/session identity. */
    config.epoch = EXAMPLE_FRAME_EPOCH;                              /* Direction/session identity. */
    config.bad_windows = EXAMPLE_FRAME_POLICY_WINDOWS;               /* One window for a short executable tutorial. */
    config.good_windows = EXAMPLE_FRAME_POLICY_WINDOWS;              /* One window for a short executable tutorial. */
    config.cooldown_windows = 0;                                     /* Demo overrides, not production tuning advice. */
    config.min_expected = 1;                                         /* Demo overrides, not production tuning advice. */
    state->initial_profile = profiles[1];                            /* Keep a public copied value rather than inspect private policy state. */
    return fvm_sadapt_init(&state->adaptation, &config) == FVM_S_OK; /* Stable state owns its copied ladder. */
} /* End bounded host-feedback replay initialization. */


/* Observe only callback-owned pixels; the cache copies them before any caller uses a lease. */
static FVM_Result observe(const FVM_DesktopFrame *frame, void *raw)                             /* Called by the VM/raster owner. */
{                                                                                               /* Called by the VM/raster owner. */
    Export *state = (Export *)raw;                                                              /* Stable native owner state through synchronous runner return. */
    FVM_RReply reply;                                                                           /* ACQUIRE metadata bytes belong to this local native copy. */
    FVM_DesktopResourceFrameInfo info;                                                          /* Exact image geometry and lease lifetime metadata. */
    FVM_SAdaptDecision decision;                                                                /* A candidate remains pending until the actual control ACK commits. */
    FVM_SControlLimits wanted;                                                                  /* A proposal never immediately changes active data geometry. */
    FVM_SControlBoundary boundary;                                                              /* New geometry begins at this exact capture sequence. */
    FVM_SControlMessage offer, ack, decoded;                                                    /* Distinct copied service records. */
    unsigned char bytes[EXAMPLE_FRAME_SLOT_LARGE];                                              /* One caller-owned output slot, never an image scratch allocation. */
    unsigned long part, count;                                                                  /* Derived from exact packed image extent and committed part quota. */
    size_t used;                                                                                /* Encoded slot extent, preserved by the library on errors. */
    int success = 0;                                                                            /* Release a successfully acquired pin on every failed export. */
    memset(&info, 0, sizeof(info));                                                             /* A zero lease means no cleanup is needed yet. */
    if (!frame || frame->width != EXAMPLE_FRAME_WIDTH || frame->height != EXAMPLE_FRAME_HEIGHT) /* Condition for this action: Fixture contract. */
    {                                                                                           /* Begin conditional branch: Condition for this action: Fixture contract. */
        return FVM_INVALID_ARGUMENT;                                                            /* Fixture contract. */
    } /* End conditional branch: Condition for this action: Fixture contract. */
    if (fvm_desktop_resource_frame_capture(state->frames, frame) != FVM_R_OK) /* Condition for this action: Copy before metadata access. */
    {                                                                         /* Begin conditional branch: Condition for this action: Copy before metadata access. */
        return FVM_ERROR;                                                     /* Copy before metadata access. */
    } /* End conditional branch: Condition for this action: Copy before metadata access. */
    if (!call(state, FVM_DESKTOP_RFRAME_ACQUIRE, 0, 0, &reply)) /* Condition for this action: Pin the copied current frame. */
    {                                                           /* Begin conditional branch: Condition for this action: Pin the copied current frame. */
        return FVM_ERROR;                                       /* Pin the copied current frame. */
    } /* End conditional branch: Condition for this action: Pin the copied current frame. */
    if (fvm_desktop_resource_frame_read(reply.data, reply.size, &info) != FVM_R_OK) /* Condition for this action: Owned metadata, not borrowed pointers. */
    {                                                                               /* Begin conditional branch: Condition for this action: Owned metadata, not borrowed pointers. */
        goto cleanup;                                                               /* Owned metadata, not borrowed pointers. */
    } /* End conditional branch: Condition for this action: Owned metadata, not borrowed pointers. */
    memset(&decision, 0, sizeof(decision));                                                                                       /* KEEP unless an actual report requests a change. */
    if (state->adaptive)                                                                                                          /* Condition for this action: Feedback belongs to the preceding image, never the frame being exported. */
    {                                                                                                                             /* Feedback belongs to the preceding image, never the frame being exported. */
        wanted = state->initial_profile;                                                                                          /* First frame has no preceding report. */
        if (info.sequence > 1)                                                                                                    /* Condition for this action: A finite trace covers only the three later boundaries. */
        {                                                                                                                         /* A finite trace covers only the three later boundaries. */
            if (info.sequence > EXAMPLE_FRAME_REPORTS + 1 || state->reports[info.sequence - 2].expected != state->previous_parts) /* Condition for this action: Verify causal sample extent. */
            {                                                                                                                     /* Begin conditional branch: Condition for this action: Verify causal sample extent. */
                goto cleanup;                                                                                                     /* Verify causal sample extent. */
            } /* End conditional branch: Condition for this action: Verify causal sample extent. */
            if (fvm_sadapt_observe(&state->adaptation, &state->reports[info.sequence - 2], &decision) != FVM_S_OK) /* Condition for this action: Select only a proposal. */
            {                                                                                                      /* Begin conditional branch: Condition for this action: Select only a proposal. */
                goto cleanup;                                                                                      /* Select only a proposal. */
            } /* End conditional branch: Condition for this action: Select only a proposal. */
            wanted = decision.limits;                                                                                                                                  /* KEEP also returns the current copied geometry. */
            if (fvm_sadapt_feedback_write(&state->reports[info.sequence - 2], bytes, sizeof(bytes), &used) != FVM_S_OK || fwrite(bytes, 1, used, state->file) != used) /* Condition for this action: Transcript includes reverse-direction feedback. */
            {                                                                                                                                                          /* Begin conditional branch: Condition for this action: Transcript includes reverse-direction feedback. */
                goto cleanup;                                                                                                                                          /* Transcript includes reverse-direction feedback. */
            } /* End conditional branch: Condition for this action: Transcript includes reverse-direction feedback. */
        } /* No change to the current image until its ACK below. */
    } /* End conditional branch: Condition for this action: Feedback belongs to the preceding image, never the frame being exported. */
    else                                                                                                                    /* Original no-feedback mode keeps its deliberate normal/small/large sequence. */
    {                                                                                                                       /* Original no-feedback mode keeps its deliberate normal/small/large sequence. */
        wanted.tag = EXAMPLE_FRAME_TAG;                                                                                     /* Direction-local fragment declaration. */
        wanted.slot_bytes = info.sequence == 1 ? EXAMPLE_FRAME_SLOT_NORMAL : info.sequence == 2 ? EXAMPLE_FRAME_SLOT_SMALL  /* Scripted profile. */
                                                                                                : EXAMPLE_FRAME_SLOT_LARGE; /* Scripted profile. */
        wanted.part_bytes = info.sequence == 1 ? EXAMPLE_FRAME_PART_NORMAL : info.sequence == 2 ? EXAMPLE_FRAME_PART_SMALL  /* Corresponding raw quota. */
                                                                                                : EXAMPLE_FRAME_PART_LARGE; /* Corresponding raw quota. */
        wanted.record_limit = wanted.slot_bytes - FVM_SCONTROL_FRAMING;                                                     /* Encoded framing remains separate. */
    } /* Both modes share the same real negotiation and retained-frame writer. */
    if (fvm_srecord_write(EXAMPLE_FRAME_METADATA_TAG, reply.data, reply.size, bytes, sizeof(bytes), &used) != FVM_S_OK || fwrite(bytes, 1, used, state->file) != used) /* Condition for this action: Preserve dimensions with object identity. */
    {                                                                                                                                                                  /* Begin conditional branch: Condition for this action: Preserve dimensions with object identity. */
        goto cleanup;                                                                                                                                                  /* Preserve dimensions with object identity. */
    } /* End conditional branch: Condition for this action: Preserve dimensions with object identity. */
    boundary.stream = EXAMPLE_FRAME_STREAM;                                                                                                  /* Future/current new image boundary. */
    boundary.epoch = EXAMPLE_FRAME_EPOCH;                                                                                                    /* Future/current new image boundary. */
    boundary.object = info.sequence;                                                                                                         /* Future/current new image boundary. */
    if (fvm_scontrol_propose_at(&state->sender, info.sequence, &wanted, &boundary, &offer) != FVM_S_OK || !service(state, &offer, &decoded)) /* Condition for this action: Pending offer only. */
    {                                                                                                                                        /* Begin conditional branch: Condition for this action: Pending offer only. */
        goto cleanup;                                                                                                                        /* Pending offer only. */
    } /* End conditional branch: Condition for this action: Pending offer only. */
    if (fvm_scontrol_respond(&state->receiver, &decoded, &ack) != FVM_S_OK || !service(state, &ack, &decoded)) /* Condition for this action: ACK stays compact. */
    {                                                                                                          /* Begin conditional branch: Condition for this action: ACK stays compact. */
        goto cleanup;                                                                                          /* ACK stays compact. */
    } /* End conditional branch: Condition for this action: ACK stays compact. */
    if (fvm_scontrol_commit(&state->receiver, &ack) != FVM_S_OK || fvm_scontrol_commit(&state->sender, &decoded) != FVM_S_OK) /* Condition for this action: Both ends agree before data. */
    {                                                                                                                         /* Begin conditional branch: Condition for this action: Both ends agree before data. */
        goto cleanup;                                                                                                         /* Both ends agree before data. */
    } /* End conditional branch: Condition for this action: Both ends agree before data. */
    if (fvm_scontrol_active(&state->sender, &wanted, &boundary) != FVM_S_OK) /* Condition for this action: Read only selected committed values. */
    {                                                                        /* Begin conditional branch: Condition for this action: Read only selected committed values. */
        goto cleanup;                                                        /* Read only selected committed values. */
    } /* End conditional branch: Condition for this action: Read only selected committed values. */
    if (state->adaptive && (decision.action == FVM_SADAPT_SHRINK || decision.action == FVM_SADAPT_GROW) && fvm_sadapt_commit(&state->adaptation, &wanted) != FVM_S_OK) /* Condition for this action: Confirm exact geometry only after the reliable ACK. */
    {                                                                                                                                                                  /* Begin conditional branch: Condition for this action: Confirm exact geometry only after the reliable ACK. */
        goto cleanup;                                                                                                                                                  /* Confirm exact geometry only after the reliable ACK. */
    } /* End conditional branch: Condition for this action: Confirm exact geometry only after the reliable ACK. */
    count = (info.byte_count - 1) / wanted.part_bytes + 1;                                                                                                                                                              /* Never derive part count from an unaccepted proposal. */
    for (part = 0; part < count; ++part)                                                                                                                                                                                /* Several slots may share the reliable file/9P data body. */
    {                                                                                                                                                                                                                   /* Several slots may share the reliable file/9P data body. */
        if (fvm_desktop_resource_frame_write_part(state->frames, EXAMPLE_FRAME_SESSION, info.lease, &state->sender, state->mode, info.sequence * EXAMPLE_FRAME_TIME_MS, part, bytes, sizeof(bytes), &used) != FVM_R_OK) /* Condition for this action: Direct retained-range encoding. */
        {                                                                                                                                                                                                               /* Begin conditional branch: Condition for this action: Direct retained-range encoding. */
            goto cleanup;                                                                                                                                                                                               /* Direct retained-range encoding. */
        } /* End conditional branch: Condition for this action: Direct retained-range encoding. */
        if (fwrite(bytes, 1, used, state->file) != used) /* Condition for this action: File I/O belongs only to this adapter. */
        {                                                /* Begin conditional branch: Condition for this action: File I/O belongs only to this adapter. */
            goto cleanup;                                /* File I/O belongs only to this adapter. */
        } /* End conditional branch: Condition for this action: File I/O belongs only to this adapter. */
        if (!part)                                      /* Condition for this action: Preserve a small final-slot witness for teardown checks. */
        {                                               /* Begin conditional branch: Condition for this action: Preserve a small final-slot witness for teardown checks. */
            state->first_hash = hash_slot(bytes, used); /* Preserve a small final-slot witness for teardown checks. */
        } /* End conditional branch: Condition for this action: Preserve a small final-slot witness for teardown checks. */
    } /* All parts retain the same capture sequence and negotiated profile. */
    state->previous_parts = count;                               /* Next feedback window must describe exactly this many distinct parts. */
    ++state->count;                                              /* Count only a completely encoded image. */
    success = 1;                                                 /* Count only a completely encoded image. */
cleanup:                                                         /* The runner cleans VM/window/audio independently of these native leases. */
    if (info.lease.id && success && state->count == state->goal) /* Condition for this action: Keep final frame after SDK shutdown. */
    {                                                            /* Begin conditional branch: Condition for this action: Keep final frame after SDK shutdown. */
        state->held = info.lease;                                /* Keep final frame after SDK shutdown. */
    } /* End conditional branch: Condition for this action: Keep final frame after SDK shutdown. */
    else if (info.lease.id && !release(state, info.lease)) /* Release on failure and earlier presentations. */
    {                                                      /* Begin conditional branch: Release on failure and earlier presentations. */
        success = 0;                                       /* Release on failure and earlier presentations. */
    } /* End conditional branch: Release on failure and earlier presentations. */
    return success ? FVM_OK : FVM_ERROR; /* Never retain callback-owned memory. */
} /* End native frame observer. */


/* Declare independent direction-local types before any metadata, service or bulk data records. */
static int declarations(Export *state)                                                                                                           /* A real peer applies these to its RX binding table. */
{                                                                                                                                                /* A real peer applies these to its RX binding table. */
    FVM_SBinding types[2];                                                                                                                       /* Two immutable type associations for this tutorial's file direction. */
    unsigned char payload[256], bytes[320];                                                                                                      /* Compact bounded declaration storage. */
    size_t used, encoded;                                                                                                                        /* Exact payload and outer S-record sizes. */
    memset(types, 0, sizeof(types));                                                                                                             /* Stable copied identity strings, no pointers on wire. */
    types[0].tag = EXAMPLE_FRAME_TAG;                                                                                                            /* Typed pixel-fragment schema. */
    types[0].version = FVM_SMEDIA_VERSION;                                                                                                       /* Typed pixel-fragment schema. */
    types[0].type_size = (unsigned int)strlen(FVM_SMEDIA_TYPE);                                                                                  /* Printable portable name. */
    strcpy(types[0].type, FVM_SMEDIA_TYPE);                                                                                                      /* Printable portable name. */
    types[1].tag = EXAMPLE_FRAME_METADATA_TAG;                                                                                                   /* Existing copied-frame schema. */
    types[1].version = FVM_DESKTOP_RFRAME_VERSION;                                                                                               /* Existing copied-frame schema. */
    types[1].type_size = (unsigned int)strlen(EXAMPLE_FRAME_METADATA_TYPE);                                                                      /* No implicit permissions. */
    strcpy(types[1].type, EXAMPLE_FRAME_METADATA_TYPE);                                                                                          /* No implicit permissions. */
    return fvm_sbindings_payload_write(types, 2, payload, sizeof(payload), &used) == FVM_S_OK &&                                                 /* Legacy tag-zero binding service. */
           fvm_srecord_write(0, payload, used, bytes, sizeof(bytes), &encoded) == FVM_S_OK && fwrite(bytes, 1, encoded, state->file) == encoded; /* Separate from limits service. */
} /* End type agreement fixture. */


/* CLI supplies one path; every reusable playback/export operation has an ordinary C interface. */
int main(int argc, char **argv)                                                                                                                            /* Rendering is offscreen and requires no native video driver. */
{                                                                                                                                                          /* Rendering is offscreen and requires no native video driver. */
    Export state;                                                                                                                                          /* Native state remains alive until all callbacks and leases are released. */
    FVM_SControlLimits ceiling = {EXAMPLE_FRAME_TAG, EXAMPLE_FRAME_SLOT_LARGE - FVM_SCONTROL_FRAMING, EXAMPLE_FRAME_SLOT_LARGE, EXAMPLE_FRAME_PART_LARGE}; /* Real output buffer capacity. */
    FVM_DesktopOptions options;                                                                                                                            /* Additive host descriptors. */
    FVM_DesktopSource source;                                                                                                                              /* Additive host descriptors. */
    FVM_DesktopFrameObserver observer;                                                                                                                     /* Additive host descriptors. */
    unsigned char bytes[EXAMPLE_FRAME_SLOT_LARGE];                                                                                                         /* Caller output for post-SDK export verification. */
    const char *error = 0, *path = 0, *report_path = 0;                                                                                                    /* Preserve failure through cleanup. */
    size_t used;                                                                                                                                           /* Preserve failure through cleanup. */
    int status = 1, live = 0, i;                                                                                                                           /* Preserve failure through cleanup. */
    if (argc == 2 && !strcmp(argv[1], "--help"))                                                                                                           /* Condition for this action: Help never opens files or initializes SDK state. */
    {                                                                                                                                                      /* Help never opens files or initializes SDK state. */
        printf("Usage: %s [--live] [--feedback reports.srecords] output.sparts\n", argv[0]);                                                               /* Explicit compact binary trace input. */
        return 0;                                                                                                                                          /* Explicit compact binary trace input. */
    } /* Parse only this replaceable executable's adapter options. */
    for (i = 1; i < argc; ++i)                   /* Reject unknown, duplicate and incomplete options before opening output. */
    {                                            /* Reject unknown, duplicate and incomplete options before opening output. */
        if (!strcmp(argv[i], "--live") && !live) /* Condition for this action: LIVE changes repair policy, not the output adapter. */
        {                                        /* Begin conditional branch: Condition for this action: LIVE changes repair policy, not the output adapter. */
            live = 1;                            /* LIVE changes repair policy, not the output adapter. */
        } /* End conditional branch: Condition for this action: LIVE changes repair policy, not the output adapter. */
        else if (!strcmp(argv[i], "--feedback") && !report_path && i + 1 < argc && argv[i + 1][0] != '-') /* Borrow input path through validation. */
        {                                                                                                 /* Begin conditional branch: Borrow input path through validation. */
            report_path = argv[++i];                                                                      /* Borrow input path through validation. */
        } /* End conditional branch: Borrow input path through validation. */
        else if (argv[i][0] != '-' && !path) /* Exactly one destination path. */
        {                                    /* Begin conditional branch: Exactly one destination path. */
            path = argv[i];                  /* Exactly one destination path. */
        } /* End conditional branch: Exactly one destination path. */
        else          /* No output side effect for invalid CLI syntax. */
        {             /* Begin conditional branch: No output side effect for invalid CLI syntax. */
            return 2; /* No output side effect for invalid CLI syntax. */
        } /* End conditional branch: No output side effect for invalid CLI syntax. */
    } /* All reusable operations below are ordinary C calls. */
    if (!path || (report_path && !strcmp(path, report_path))) /* Condition for this action: Do not overwrite the same named input. */
    {                                                         /* Begin conditional branch: Condition for this action: Do not overwrite the same named input. */
        return 2;                                             /* Do not overwrite the same named input. */
    } /* End conditional branch: Condition for this action: Do not overwrite the same named input. */
    memset(&state, 0, sizeof(state));                                              /* Partial initialization is safe for owner cleanup. */
    state.adaptive = report_path != 0;                                             /* Four feedback-driven or three scripted frames. */
    state.goal = state.adaptive ? EXAMPLE_FRAME_REPORTS + 1 : EXAMPLE_FRAME_COUNT; /* Four feedback-driven or three scripted frames. */
    if (report_path && !feedback(&state, report_path))                             /* Condition for this action: Decode all input before touching the destination. */
    {                                                                              /* Begin conditional branch: Condition for this action: Decode all input before touching the destination. */
        return 1;                                                                  /* Decode all input before touching the destination. */
    } /* End conditional branch: Condition for this action: Decode all input before touching the destination. */
    state.mode = live ? FVM_SMEDIA_LIVE : FVM_SMEDIA_FILE; /* Explicit application policy. */
    state.file = fopen(path, "wb");                        /* No partial native cache exists yet. */
    if (!state.file)                                       /* Condition for this action: No partial native cache exists yet. */
    {                                                      /* Begin conditional branch: Condition for this action: No partial native cache exists yet. */
        return 1;                                          /* No partial native cache exists yet. */
    } /* End conditional branch: Condition for this action: No partial native cache exists yet. */
    if (fvm_rdispatch_create(0, &state.dispatch) != FVM_R_OK || fvm_desktop_resource_frame_create(state.dispatch, "/display/frame", EXAMPLE_FRAME_BYTES, &state.frames, &state.target) != FVM_R_OK) /* Condition for this action: Explicit fixed cache budget. */
    {                                                                                                                                                                                               /* Begin conditional branch: Condition for this action: Explicit fixed cache budget. */
        goto cleanup;                                                                                                                                                                               /* Explicit fixed cache budget. */
    } /* End conditional branch: Condition for this action: Explicit fixed cache budget. */
    if (fvm_rdispatch_grant(state.dispatch, state.target, EXAMPLE_FRAME_SESSION, FVM_R_READ) != FVM_R_OK || fvm_scontrol_init(&state.sender, &ceiling) != FVM_S_OK || fvm_scontrol_init(&state.receiver, &ceiling) != FVM_S_OK || !declarations(&state)) /* Condition for this action: No hidden grant/session. */
    {                                                                                                                                                                                                                                                    /* Begin conditional branch: Condition for this action: No hidden grant/session. */
        goto cleanup;                                                                                                                                                                                                                                    /* No hidden grant/session. */
    } /* End conditional branch: Condition for this action: No hidden grant/session. */
    fvm_desktop_options_init(&options);                                                                            /* Deterministic fixture, no audio device. */
    options.mute = 1;                                                                                              /* Deterministic fixture, no audio device. */
    options.frames = state.goal;                                                                                   /* Deterministic fixture, no audio device. */
    options.mode = FVM_EXECUTION_INTERPRETER;                                                                      /* Deterministic fixture, no audio device. */
    source.kind = FVM_DESKTOP_SOURCE_MEMORY;                                                                       /* Count embedded binary bytes. */
    source.location = "frame-parts.swf";                                                                           /* Count embedded binary bytes. */
    source.bytes = embedded_movie;                                                                                 /* Count embedded binary bytes. */
    source.size = sizeof(embedded_movie);                                                                          /* Count embedded binary bytes. */
    fvm_desktop_frame_observer_init(&observer);                                                                    /* Borrow owner state synchronously. */
    observer.notify = observe;                                                                                     /* Borrow owner state synchronously. */
    observer.userdata = &state;                                                                                    /* Borrow owner state synchronously. */
    if (fvm_desktop_run_source_offscreen(&options, &source, 0, 0, &observer, &error) || state.count != state.goal) /* Condition for this action: Real VM and raster execution. */
    {                                                                                                              /* Begin conditional branch: Condition for this action: Real VM and raster execution. */
        goto cleanup;                                                                                              /* Real VM and raster execution. */
    } /* End conditional branch: Condition for this action: Real VM and raster execution. */
    if (fvm_desktop_shutdown() != FVM_OK) /* Condition for this action: No VM/SDK owner remains below. */
    {                                     /* Begin conditional branch: Condition for this action: No VM/SDK owner remains below. */
        goto cleanup;                     /* No VM/SDK owner remains below. */
    } /* End conditional branch: Condition for this action: No VM/SDK owner remains below. */
    if (fvm_desktop_resource_frame_write_part(state.frames, EXAMPLE_FRAME_SESSION, state.held, &state.sender, state.mode, state.goal * EXAMPLE_FRAME_TIME_MS, 0, bytes, sizeof(bytes), &used) != FVM_R_OK || hash_slot(bytes, used) != state.first_hash) /* Condition for this action: Final pinned bytes survive teardown. */
    {                                                                                                                                                                                                                                                    /* Begin conditional branch: Condition for this action: Final pinned bytes survive teardown. */
        goto cleanup;                                                                                                                                                                                                                                    /* Final pinned bytes survive teardown. */
    } /* End conditional branch: Condition for this action: Final pinned bytes survive teardown. */
    status = 0;                                                                                             /* Output will be accepted only if final cleanup/close also succeeds. */
cleanup:                                                                                                    /* Quiesced native cleanup never dereferences a dead context or runner frame. */
    if (state.frames && fvm_desktop_resource_frame_revoke(state.frames, EXAMPLE_FRAME_SESSION) != FVM_R_OK) /* Condition for this action: Release every pin even on an incomplete file. */
    {                                                                                                       /* Begin conditional branch: Condition for this action: Release every pin even on an incomplete file. */
        status = 1;                                                                                         /* Release every pin even on an incomplete file. */
    } /* End conditional branch: Condition for this action: Release every pin even on an incomplete file. */
    if (state.frames && fvm_desktop_resource_frame_destroy(&state.frames) != FVM_R_OK) /* Condition for this action: Free three native cache slots. */
    {                                                                                  /* Begin conditional branch: Condition for this action: Free three native cache slots. */
        status = 1;                                                                    /* Free three native cache slots. */
    } /* End conditional branch: Condition for this action: Free three native cache slots. */
    if (state.dispatch && fvm_rdispatch_destroy(&state.dispatch) != FVM_R_OK) /* Condition for this action: No accepted jobs may remain. */
    {                                                                         /* Begin conditional branch: Condition for this action: No accepted jobs may remain. */
        status = 1;                                                           /* No accepted jobs may remain. */
    } /* End conditional branch: Condition for this action: No accepted jobs may remain. */
    if (fvm_desktop_shutdown() != FVM_OK) /* Condition for this action: Repeated quiesced shutdown is supported. */
    {                                     /* Begin conditional branch: Condition for this action: Repeated quiesced shutdown is supported. */
        status = 1;                       /* Repeated quiesced shutdown is supported. */
    } /* End conditional branch: Condition for this action: Repeated quiesced shutdown is supported. */
    if (fclose(state.file)) /* Condition for this action: Report delayed filesystem errors. */
    {                       /* Begin conditional branch: Condition for this action: Report delayed filesystem errors. */
        status = 1;         /* Report delayed filesystem errors. */
    } /* End conditional branch: Condition for this action: Report delayed filesystem errors. */
    if (status)       /* Condition for this action: Do not leave a partial output created by this invocation. */
    {                 /* Begin conditional branch: Condition for this action: Do not leave a partial output created by this invocation. */
        remove(path); /* Do not leave a partial output created by this invocation. */
    } /* End conditional branch: Condition for this action: Do not leave a partial output created by this invocation. */
    else                                              /* Stable result, no binary bytes printed. */
    {                                                 /* Begin conditional branch: Stable result, no binary bytes printed. */
        printf("exported_frames=%lu\n", state.count); /* Stable result, no binary bytes printed. */
    } /* End conditional branch: Stable result, no binary bytes printed. */
    return status; /* C hosts can replace the file adapter with a bounded channel queue. */
} /* End executable tutorial. */
