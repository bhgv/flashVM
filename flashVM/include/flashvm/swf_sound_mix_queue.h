/* Native event/stream composition adapter retaining exact mixed bytes across queue rejection. */
#ifndef FLASHVM_SWF_SOUND_MIX_QUEUE_H
#define FLASHVM_SWF_SOUND_MIX_QUEUE_H /* Guard the standalone C89 adapter ABI. */
#include "swf_sound_output.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Native owner, borrowing a live mixer and the exclusive all-or-none device backend. */
    typedef struct FVM_SWFSoundMixQueue FVM_SWFSoundMixQueue;
    /* Immutable capacity and current retry ownership; no device observation is implied. */
    typedef struct FVM_SWFSoundMixQueueInfo
    {
        size_t struct_size;     /* Caller supplies exact native sizeof. */
        size_t capacity_frames; /* Maximum input frames in one callback, fixed at creation. */
        size_t pending_frames;  /* Mixed and voice-advanced frames awaiting real acceptance. */
    } FVM_SWFSoundMixQueueInfo;
#define FVM_SWF_SOUND_MIX_QUEUE_PENDING_NONE 0U /* No rendered retry is owned. */
#define FVM_SWF_SOUND_MIX_QUEUE_PENDING_BASE 1U /* Retry belongs to adapted.write input. */
#define FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL 2U /* Retry belongs to pump_tail. */
    /* Additive ownership observation; existing Info structure and ABI stay unchanged. */
    typedef struct FVM_SWFSoundMixQueuePending
    {
        size_t struct_size; /* Caller supplies exact sizeof. */
        size_t frames; /* Rendered frames awaiting real acceptance, zero when idle. */
        unsigned int origin; /* PENDING_NONE, PENDING_BASE or PENDING_TAIL. */
    } FVM_SWFSoundMixQueuePending;
    /* Copy pending ownership without polling backend, allocating or changing voice time.
     * Required live queue/out with exact size; invalid/reentry -> INVALID_ARGUMENT,
     * output unchanged. Serialize queue/mixer externally; no VM/GC or reservation. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_pending(const FVM_SWFSoundMixQueue *,
                                                       FVM_SWFSoundMixQueuePending *);
    /* Exact native allocation requests only, excluding allocator/mixer/voice/device/VM memory. */
    typedef struct FVM_SWFSoundMixQueueStorageInfo
    {
        size_t struct_size; /* Caller supplies exact sizeof. */
        size_t fixed_bytes; /* Native owner including its vector descriptors. */
        size_t buffer_bytes; /* Original/mixed S16 arrays plus optional fixed identity deadlines. */
        size_t retained_bytes; /* Checked sum of fixed and buffer bytes. */
    } FVM_SWFSoundMixQueueStorageInfo;
    /* Predict initial untracked adapter bytes without allocating or calling VM/backend/mixer.
     * channels 1/2 and block 1..OUTPUT_MAX_BLOCK required; invalid -> INVALID_ARGUMENT,
     * size overflow -> OUT_OF_MEMORY. Required scalar out is preserved on errors. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_storage_required(unsigned int channels,
                                                                size_t block, size_t *out);
    /* Predict owner+PCM+optional ID-table allocation requests before acquiring any owner.
     * identity_tracking must be 0/1; geometry and statuses match storage_required. Required
     * out is unchanged on invalid arguments or checked size overflow. No allocation,
     * mixer/backend/VM/GC access; thread independent when out storage is not shared.
     * This is retained adapter bytes, not a combined mixer/voice/device/allocator budget.
     * Creation remains initially untracked; enable tracking separately at its safe point. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_storage_required_for_tracking(
        unsigned int channels, size_t block, int identity_tracking, size_t *out);
#define FVM_SWF_SOUND_MIX_QUEUE_ORIGIN_FRAMES FVM_SWF_SOUND_OUTPUT_MAX_QUEUE /* Fixed accepted FIFO origin capacity. */
    /* Predict owner+PCM+optional ID and BASE/TAIL origin tracking allocation requests.
     * Both mode flags are 0/1; same geometry and checked-byte error rules as storage_required.
     * Required out unchanged on errors. Origin mode adds a fixed one-bit-per-FIFO-frame
     * bitmap; excludes mixer/voice/backend/VM/allocator memory. No allocation, callbacks,
     * VM or GC. This prediction does not enable either optional mode. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_storage_required_for_modes(
        unsigned int channels, size_t block, int identities, int origins, size_t *out);
    /* Copy fixed storage observation, independent of pending/accepted queue frames.
     * Live serialized queue and exact-size out required; invalid/reentry preserves out.
     * No allocation/callback/VM/GC. Counts allocation requests, not actual RSS. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_storage_info(const FVM_SWFSoundMixQueue *,
                                                            FVM_SWFSoundMixQueueStorageInfo *);
    /* Shared accepted-minus-queued observation for base and tail writes in one explicit epoch. */
    typedef struct FVM_SWFSoundMixQueueClock
    {
        size_t struct_size; /* Caller supplies exact sizeof. */
        double submitted_frames; /* Real accepted base+tail frames, exact through OUTPUT_MAX_CLOCK. */
        double played_frames; /* Submitted minus observed backlog, monotone within this epoch. */
        double seconds; /* Dequeued frames divided by the adapter output sample rate; not a DAC clock. */
        size_t queued_frames; /* Fresh validated backend backlog in complete output frames. */
    } FVM_SWFSoundMixQueueClock;
    /* Start/restart shared accounting only with no pending retry and an empty real FIFO.
     * Required live serialized owner; INVALID_ARGUMENT before callbacks for null/busy/pending.
     * Nonempty FIFO INVALID_ARGUMENT, inadequate double precision UNSUPPORTED, backend
     * statuses propagate, exceptions ERROR. Failure preserves old epoch; success resets to
     * zero and enables accounting of every later adapted.write and pump_tail acceptance.
     * No pause/clear/seek/voice reset, allocation, VM or GC. Retain all borrowed owners;
     * use only the adapted backend until quiescence. Acknowledged adapted.clear starts a
     * fresh zero epoch while keeping accounting enabled. Create remains callback-free and
     * leaves shared accounting disabled until this explicit call. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_clock_begin(FVM_SWFSoundMixQueue *);
    /* End opt-in accounting only after all producers are quiescent and FIFO has drained.
     * Pending retry/null/busy -> INVALID_ARGUMENT before callbacks; inactive -> NOT_FOUND.
     * Poll validated backlog; nonempty -> NOT_FOUND with accounting still enabled.
     * Backend/error/exception rules match clock_info; a valid poll may advance the last
     * played observation even when not drained. Success disables accounting, no pause,
     * clear, voice/converter change or allocation/VM/GC. Subsequent clock_info is NOT_FOUND;
     * ordinary adapted callbacks return to their legacy unclocked contract. Retain and
     * serialize all owners; this is not an automatic backend ownership transfer. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_clock_end(FVM_SWFSoundMixQueue *);
    /* Observe a fresh shared clock without consuming voice time or allocating. Required
     * exact-size output is unchanged on failure. NOT_FOUND before clock_begin. Backend
     * failures/exception preserve output and last valid played position; impossible backlog
     * or played regression -> ERROR. Reentry INVALID_ARGUMENT. Serialize all owners; no
     * VM/GC. Rejected writes never add submitted frames; retained retries count once on OK.
     * MAX_CLOCK overflow is rejected with UNSUPPORTED before rendering/writing the block.
     * Clock exhaustion is not source EOF; queued drain is not a physical playback timestamp. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_clock_info(FVM_SWFSoundMixQueue *,
                                                          FVM_SWFSoundMixQueueClock *);
#define FVM_SWF_SOUND_MIX_QUEUE_ID_SLOTS 65536UL /* Optional fixed sound-ID deadlines, including unused zero. */
    /* Opt in/out of queued sound identities. enabled is 0/1; repeated selection is an
     * allocation-free no-op. Require active shared clock, no pending/mixer voices; enable
     * only in a zero epoch, disable only after observed drain and expired registrations.
     * On enable, adapter_budget covers native owner, both PCM arrays and ID_SLOTS doubles;
     * insufficient/overflow -> OUT_OF_MEMORY before allocation, failure preserves old mode.
     * No backend callback/VM/GC. One root dictionary ID namespace per epoch; child dictionaries are not remapped.
     * ALL voice starts/stops must use this queue's tracked APIs;
     * direct mixer mutation invalidates the accounting contract. While enabled, clock_begin/
     * clock_end reject INVALID_ARGUMENT; disable first at a drained safe point.
     * Opt-in adapted.clear stops every mixer voice and resets deadlines/clock/pending only
     * AFTER real clear acknowledgement; failed clear preserves them. Legacy untracked clear
     * still does not stop the mixer. Serialize all borrowed owners and queue callbacks. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_set_identity_tracking(FVM_SWFSoundMixQueue *,
                                                                     int enabled,
                                                                     size_t adapter_budget);
    /* Adopt a prepared voice with queued SyncNoMultiple semantics; tracking must be enabled.
     * Observe FIFO, include retained pending frames before the new start, then register exact
     * converted duration at phase zero. NoMultiple consumes/closes/clears an unused voice
     * while same-ID registered time is unplayed, even after mixer retirement. Ordinary
     * same-ID starts overlap and extend the largest deadline; finished voices add none.
     * Success consumes *voice; errors preserve ownership/progress/old voices/deadlines,
     * though a valid queue poll may refresh played time. mixer_budget covers mixer/voices
     * only, excludes adapter/table/backend/VM. No allocation/VM/GC; callbacks may block.
     * Disabled UNSUPPORTED; null/busy/invalid voice INVALID_ARGUMENT; time overflow
     * UNSUPPORTED; budget/full mixer OUT_OF_MEMORY; backend errors propagate, exceptions
     * ERROR. Serialize all owners; direct mixer starts/stops invalidate tracked accounting. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_submit_prepared_bounded(FVM_SWFSoundMixQueue *,
                                                                       FVM_SWFSoundVoice **,
                                                                       size_t mixer_budget);
    /* ID deadlines use the shared accepted-minus-queued epoch, not DAC audibility. */
    typedef struct FVM_SWFSoundMixQueueIdentityInfo
    {
        size_t struct_size; /* Caller supplies exact sizeof. */
        unsigned int sound_id; /* Requested dictionary ID, 1..65535. */
        double played_frames; /* Fresh validated shared dequeue cursor. */
        double end_frame; /* Last registered exclusive end, retained historically after expiry. */
        double until_end_frames; /* max(end_frame-played_frames,0), includes uncomposed/pending time. */
        int active; /* True exactly while the registered interval remains undequeued. */
    } FVM_SWFSoundMixQueueIdentityInfo;
    /* Observe the real FIFO and copy one tracked identity, without allocation/VM/GC.
     * Required exact-size out and valid ID/live serialized queue. Invalid/reentry ->
     * INVALID_ARGUMENT; disabled UNSUPPORTED; backend statuses propagate, exceptions ERROR.
     * Errors preserve output and deadline state; valid polls may refresh played time.
     * No reservation or voice rewind; retain adapter/mixer/backend through callbacks. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_identity_info(FVM_SWFSoundMixQueue *,
                                                             unsigned int sound_id,
                                                             FVM_SWFSoundMixQueueIdentityInfo *);
    /* Boolean identity_info view; required scalar out is preserved on every failure.
     * Same callback/thread/ownership/status contract, no allocation/VM/GC. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_sound_active(FVM_SWFSoundMixQueue *,
                                                            unsigned int sound_id, int *out);
    /* Stop future same-ID mixing using the mixer's immediate/DECLICK policy, then cancel
     * its tracked deadline. Existing accepted/pending PCM is unchanged; a later start
     * composes only after that retained interval. Does not pause/clear/rewind the FIFO.
     * Required ID 1..65535/live serialized queue; invalid/reentry INVALID_ARGUMENT,
     * disabled UNSUPPORTED. No allocation/callback/VM/GC; real mixer errors propagate.
     * Keep all borrowed owners live; use adapted.clear for acknowledged full cancellation. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_stop_id(FVM_SWFSoundMixQueue *, unsigned int sound_id);
    /* Immediately retire all event voices and cancel every tracked ID deadline. This is
     * event-only cancellation: accepted/pending mixed bytes, base stream cursors, pause
     * and shared clock are unchanged. No DECLICK release is generated, matching mixer
     * stop_all. Requires enabled tracking and live externally serialized borrowed owners.
     * Null/reentry INVALID_ARGUMENT, disabled UNSUPPORTED, mixer errors propagate before
     * deadline mutation. No allocation/backend callback/VM/GC. Use adapted.clear for
     * acknowledged removal of pending/FIFO bytes; this operation never clears them. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_stop_all(FVM_SWFSoundMixQueue *);
    /* Allocate fixed original/mixed buffers; copy matching mixer/backend geometry. Owner only.
     * No callback/pause/clear/VM. Clear *out first; preserve adapted on failure. Both outputs
     * required/disjoint. Null/bad geometry or block outside 1..OUTPUT_MAX_BLOCK INVALID_ARGUMENT;
     * allocation/size overflow OUT_OF_MEMORY. Returned table borrows adapter until close.
     * Serialize table/mixer externally; only this adapter may render the borrowed mixer.
     * write copies base and mixes once. After real write failure, retry identical original
     * frames/bytes: the retained mixture is resent without advancing voices. A mismatch is
     * INVALID_ARGUMENT. Successful clear discards pending data without restoring voice time.
     * queued publishes only on OK; pause accepts 0/1. Real statuses propagate; callback
     * exceptions become ERROR. Callback reentry is INVALID_ARGUMENT, not a thread lock.
     * Wrapped calls allocate no storage and access no VM. Keep both borrowed owners alive;
     * use the adapted table exclusively for the real queue until every producer is quiescent. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_create(FVM_SWFSoundMixer *,
                                                      const FVM_AudioQueueBackend *,
                                                      size_t block_frames,
                                                      FVM_SWFSoundMixQueue **out,
                                                      FVM_AudioQueueBackend *adapted);
    /* Same ownership/validation as create, with a requested native-storage byte ceiling.
     * Validate descriptors/mixer geometry before comparing budget; zero or insufficient
     * budget -> OUT_OF_MEMORY before any allocation/callback. Counts only this owner
     * and its fixed buffers as reported by storage_required/storage_info; excludes
     * allocator, mixer/voices, VM and device. Clears *out first, preserves adapted on
     * failure, including allocation failure. No callbacks/VM/GC. Original create uses
     * an unrestricted ceiling and retains its ABI. Keep outputs live and disjoint. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_create_bounded(FVM_SWFSoundMixer *,
                                                               const FVM_AudioQueueBackend *,
                                                               size_t block_frames, size_t budget_bytes,
                                                               FVM_SWFSoundMixQueue **out,
                                                               FVM_AudioQueueBackend *adapted);
    /* Authorize one event-only zero-base block after quiescing the stream/base producer.
     * frames is 1..capacity, target is frames..SOUND_OUTPUT_MAX_QUEUE. Observe the real
     * FIFO first; insufficient headroom or no active voices/pending tail -> NOT_FOUND.
     * No voice time is consumed by headroom checks/observation failure. A rejected write
     * retains exactly mixed bytes even on backend NOT_FOUND; retry the same frames,
     * never advance voices twice. NOT_FOUND alone does not imply voice/source EOF.
     * Pending base input or a different pending tail size -> INVALID_ARGUMENT before
     * callbacks. Adapted write cannot consume a tail retry, even with identical zeros.
     * accepted is optional and cleared first; on OK it receives frames. No implicit
     * pause/clear/seek or device clock; output may include final block padding silence.
     * Queued data may remain after NOT_FOUND for exhausted voices. Serialize all owners,
     * retain mixer/backend/adapter, no allocation/VM/GC. Reentry INVALID_ARGUMENT;
     * real callback statuses propagate and exceptions become ERROR. Successful adapted
     * clear cancels pending ownership without restoring consumed voice time. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_pump_tail(FVM_SWFSoundMixQueue *,
                                                         size_t frames, size_t target,
                                                         size_t *accepted);
    /* Authorize up to maximum event-only frames in currently available FIFO headroom.
     * maximum is 1..capacity; target is 1..SOUND_OUTPUT_MAX_QUEUE, and may be smaller
     * than maximum. Fresh work uses min(maximum,target-queued); full FIFO or exhausted
     * voices -> NOT_FOUND without consuming voice time. A retained TAIL is indivisible:
     * if larger than maximum or available headroom, NOT_FOUND preserves it; otherwise
     * resend its original size/bytes exactly once, even when maximum changes. Pending
     * BASE, null/reentry/invalid geometry -> INVALID_ARGUMENT. accepted is optional,
     * cleared first, and set to actual accepted frames only on OK. Existing exact-size
     * pump_tail semantics are unchanged. Shared-clock overflow -> UNSUPPORTED before
     * rendering; real callback errors propagate, exceptions ERROR. No allocation, VM,
     * GC, pause/clear/seek or source EOF inference. Padding may fill the chosen block.
     * Quiesce base production and externally serialize all borrowed owners/callbacks;
     * use the paired stream scheduler when sharing its accepted epoch. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_pump_tail_up_to(FVM_SWFSoundMixQueue *,
                                                               size_t maximum, size_t target,
                                                               size_t *accepted);
    /* Finite adaptive variant: fresh work is also clamped to the current live mixer
     * horizon, so no block padding is appended beyond the final voice/release frame.
     * Existing pending TAIL bytes remain indivisible, including legacy padded blocks;
     * stopping voices never truncates a retry. Same maximum/target/status/output,
     * ownership, serialization and no-allocation/VM/GC contract as pump_tail_up_to.
     * Zero-valued source frames still count; no physical audibility/EOF inference. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_pump_tail_finite(FVM_SWFSoundMixQueue *,
                                                               size_t maximum, size_t target,
                                                               size_t *accepted);
    /* No allocation/callback/VM. Null/wrong-size/reentry INVALID_ARGUMENT, output intact. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_info(const FVM_SWFSoundMixQueue *,
                                                    FVM_SWFSoundMixQueueInfo *);
    /* Quiesce producer/table callbacks first; no callback, mixer/device stop or ownership transfer.
     * Pending bytes are discarded; consumed voice time is not restored. Null is a no-op. */
    FVM_API void fvm_swf_sound_mix_queue_close(FVM_SWFSoundMixQueue *);
#ifdef __cplusplus
}
#endif
#endif
