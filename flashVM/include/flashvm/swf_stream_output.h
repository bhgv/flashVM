/* Caller-buffered prepared stream PCM production into an explicitly owned native FIFO backend. */
#ifndef FLASHVM_SWF_STREAM_OUTPUT_H
#define FLASHVM_SWF_STREAM_OUTPUT_H /* Guard the standalone C89 producer ABI. */
#include "swf_stream_convert.h"
#include "swf_sound_output.h"
#include "swf_sound_mix_queue.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Owned observations of accepted data, distinct from device/DAC or movie clocks. */
    typedef struct FVM_SWFStreamOutputInfo
    {
        size_t struct_size;      /* Caller sets exact native ABI size. */
        double submitted_frames; /* Accepted in this seek epoch, exact through MAX_CLOCK. */
        double played_frames;    /* Submitted minus observed queued, not a DAC playback position. */
        double seconds;          /* Dequeued frames divided by explicit output sample rate. */
        size_t queued_frames;    /* Last validated FIFO backlog. */
        size_t pending_frames;   /* Prepared retry data not yet accepted by backend. */
        int paused;              /* Last acknowledged pause state. */
        int eof; /* Source exhausted and no pending block; queued audio may still remain. */
        FVM_SWFStreamConvertedChunk
            chunk; /* Last prepared source/output metadata, including markers. */
    } FVM_SWFStreamOutputInfo;
    /* One native FIFO observation shared by stream metadata and BASE/TAIL categories. */
    typedef struct FVM_SWFStreamMixInfo
    {
        size_t struct_size; /* Caller supplies exact size; nested sizes are initialized on OK. */
        FVM_SWFStreamOutputInfo stream; /* Source/retry metadata and the shared dequeue clock. */
        FVM_SWFSoundMixQueueOriginInfo origins; /* Categories of the same accepted/queued frames. */
    } FVM_SWFStreamMixInfo;
    /* Caller-owned serial state; no hidden allocation or VM references. Fields private after init.
     */
    typedef struct FVM_SWFStreamOutput
    {
        size_t struct_size; /* Exact ABI initialized on success. */
        FVM_SWFStreamConverter
            *converter;                /* Exclusively borrowed at its current source position. */
        FVM_AudioQueueBackend backend; /* Copied callback table, borrowed userdata/device. */
        short *pcm; /* Exclusively borrowed disjoint retry storage until producer quiescence. */
        size_t capacity; /* Accessible scalar values, at least block*output_channels. */
        size_t block;    /* Max prepared output frames, 1..CONVERT_MAX_FRAMES. */
        size_t target;   /* Requested FIFO frame ceiling, block..SOUND_OUTPUT_MAX_QUEUE. */
        FVM_SWFStreamOutputInfo info; /* Copied queue clock, pending lifetime and source marker. */
        int busy; /* Reject callback reentry; external synchronization is still required. */
    } FVM_SWFStreamOutput;
    /* Match converter and backend geometry, require an empty exclusive queue and pause it on OK.
     * Caller owns state/retry buffer/converter/snapshot/backend throughout use. No allocation.
     * Invalid descriptors INVALID_ARGUMENT, inadequate double precision UNSUPPORTED; callback
     * statuses propagate, C++ exceptions ERROR. Errors preserve state/converter/PCM. A failing
     * backend operation must obey the atomic backend contract; init reentry is not permitted. */
    FVM_API FVM_Result fvm_swf_stream_output_init(FVM_SWFStreamOutput *, FVM_SWFStreamConverter *,
                                                  const FVM_AudioQueueBackend *, short *pcm,
                                                  size_t capacity, size_t block, size_t target);
    /* Prepare/retry at most one block. accepted is optional and cleared first, including errors.
     * Backend copies all PCM before OK, accepts none on failure. Failed writes retain exact pending
     * bytes/chunk and advanced converter position; retry never decodes them twice. NOT_FOUND means
     * no queue budget or source EOF (query info.eof). Empty/skipped markers return OK/accepted=0.
     * No allocation/VM; callbacks may block only according to host policy. Reentry
     * INVALID_ARGUMENT. */
    FVM_API FVM_Result fvm_swf_stream_output_pump(FVM_SWFStreamOutput *, size_t *accepted);
    /* Copy the next pending/source block's movie frame without decoding, callbacks or VM access.
     * NOT_FOUND at source EOF; errors preserve output. Pending retry metadata takes precedence.
     * This query does not observe/drain FIFO or grant timeline authorization. */
    FVM_API FVM_Result fvm_swf_stream_output_next_frame(const FVM_SWFStreamOutput *,
                                                        unsigned int *);
    /* Same bounded producer contract, but do not prepare/accept a block beyond max_movie_frame.
     * Zero holds all source frames. Gate/backpressure/EOF return NOT_FOUND; next_frame/info
     * distinguish them. Lowering a gate cannot recall already accepted FIFO audio: backward
     * navigation requires explicit seek/clear. A disallowed pending retry remains unchanged.
     * Continuous interpolation may inspect the next immutable sample in the prepared same-header
     * source; this gate does not turn an early host stop into source EOF or disable lookahead.
     * The host supplies actual timeline authorization, never a tick count or packet timestamp. */
    FVM_API FVM_Result fvm_swf_stream_output_pump_until(FVM_SWFStreamOutput *,
                                                        unsigned int max_movie_frame,
                                                        size_t *accepted);
    /* Pump one event-only composition block after this stream has observed source EOF.
     * Requires no base retry, the exact adapted backend of mix, an active shared clock
     * and identical submitted epochs. Uses this producer's block/target and updates its
     * accepted/dequeued accounting on success; source EOF/chunk/converter stay unchanged.
     * No struct ABI change. Continue/seek/pause/info remain valid after paired tails.
     * Settle any mix TAIL retry before continue; successful adapted seek/clear cancels it.
     * Never interleave direct mix-tail/base writes: use this operation for all EOF tails.
     * accepted is optional and cleared first. Invalid/reentry/mismatched owners or epochs:
     * INVALID_ARGUMENT; missing shared clock: NOT_FOUND; mixer headroom/exhaustion:
     * NOT_FOUND. Other statuses propagate; backend exceptions become ERROR.
     * Failed writes retain the mix queue's exact TAIL retry, not this producer's base PCM.
     * Query mix_queue_pending for that retry; StreamOutputInfo.pending_frames remains base-only.
     * No allocation/VM/GC; externally serialize both owners, mixer and exclusive backend.
     * Keep all borrowed owners alive; this operation does not pause, clear or authorize
     * future stream frames. Only call after the host has chosen final sequence EOF. */
    FVM_API FVM_Result fvm_swf_stream_output_pump_mix_tail(FVM_SWFStreamOutput *,
        FVM_SWFSoundMixQueue *, size_t *accepted);
    /* Coordinate one mixed block through a host-selected movie-frame gate. Zero holds all
     * base source frames but still permits event-only output. Prefer authorized base; when
     * its next block is gated or source EOF, mix voices over zero base. Silence is not
     * emitted without active voices or an existing mixed retry. Uses producer block/target.
     * Retained TAIL retry is accepted before newly authorized base, unchanged by gate moves;
     * retained BASE retry stays held if its source frame is disallowed. Never reinterpret
     * retries or advance their voices twice. Same exact backend/shared-clock pairing,
     * lifetime/serialization and statuses as pump_mix_tail, but observed EOF is not required.
     * accepted optional/cleared first; OK may consume a zero-frame source marker. NOT_FOUND
     * can mean backpressure, exhaustion, missing clock or held BASE retry: inspect both
     * producers. On failed write, keep all owners and retry this operation; seek/clear may
     * cancel only after acknowledgement. No allocation/VM/GC, implicit pause or A/V retiming.
     * Successful event-only blocks count in both clocks, preserve source cursor/chunk/EOF;
     * output padding and already queued lead are not reference Flash timestamp semantics. */
    /* Fresh event-only blocks fit available FIFO headroom up to block; a retained short
     * TAIL remains indivisible and precedes newly permitted base. Copied queue/played
     * counters use the successful write admission observation without a post-write poll. */
    FVM_API FVM_Result fvm_swf_stream_output_pump_mix_until(FVM_SWFStreamOutput *,
        FVM_SWFSoundMixQueue *, unsigned int max_movie_frame, size_t *accepted);
    /* Acknowledge pause/resume without clearing, priming or advancing converter position. */
    FVM_API FVM_Result fvm_swf_stream_output_pause(FVM_SWFStreamOutput *, int paused);
    /* Opt-in finite variant of pump_mix_until: clamp only fresh event-only work to
     * the live voice/release horizon, avoiding final synthetic block padding. Base,
     * source-frame gates, common epoch validation and indivisible BASE/TAIL retries
     * keep the same contract, including optional accepted cleared first. Previously
     * rendered padded TAIL bytes are never trimmed. Same statuses, external owner
     * serialization and no-allocation/callback-after-write/VM/GC restrictions. */
    FVM_API FVM_Result fvm_swf_stream_output_pump_mix_finite_until(FVM_SWFStreamOutput *,
        FVM_SWFSoundMixQueue *, unsigned int max_movie_frame, size_t *accepted);
    /* Observe backend backlog and copy clock/metadata; failures preserve caller output. Reject
     * impossible growth beyond submitted/target or regression of played_frames with ERROR.
     * Requires exclusive producer and queue ownership, including no independent clears/writes. */
    FVM_API FVM_Result fvm_swf_stream_output_info(FVM_SWFStreamOutput *, FVM_SWFStreamOutputInfo *);
    /* Observe a paired stream/mix through exactly one real queued callback. Require the exact
     * adapted backend, equal submitted epochs and enabled origin tracking. Copies stream and
     * category clocks from that same observation; updates stream's cached clock only on OK.
     * Does not decode, render, advance source, allocate, authorize frames or call VM/GC.
     * Retained BASE/TAIL bytes are excluded from submitted/queued, as with separate queries.
     * INVALID_ARGUMENT for invalid size/reentry/owners/epochs or queue beyond stream target;
     * UNSUPPORTED when origin mode is disabled; backend errors propagate, exceptions ERROR.
     * Errors preserve out and stream cache; a valid mix dequeue observation may still retire
     * its native ledger. No rollback of device progress. Keep all borrowed owners alive and
     * externally serialize both producers, converter, mixer and exclusive FIFO. Not DAC time
     * or a mapping from output samples to source/movie timestamps. Nested sizes need not be set. */
    FVM_API FVM_Result fvm_swf_stream_output_mix_info(FVM_SWFStreamOutput *,
        FVM_SWFSoundMixQueue *, FVM_SWFStreamMixInfo *out);
    /* Validate absolute source seek first, pause then clear FIFO, discard pending and reset clock
     * epoch only after successful clear. New grid phase zero. Pause success remains visible if
     * clear fails; pending/converter/epoch stay intact for retry. Errors cannot undo host effects.
     * Leave queue paused; caller explicitly primes/resumes. EOF seek supported. */
    FVM_API FVM_Result fvm_swf_stream_output_seek(FVM_SWFStreamOutput *, size_t block,
                                                  size_t first);
    /* Map a positive movie frame to the first indexed source block at/after it, then perform
     * staged pause/clear/seek with phase zero. Gaps wait for a later block; beyond the last
     * block seeks EOF. This is a prefix discard policy, not reference Flash seek emulation.
     * Binary metadata lookup only; errors before seek cause no backend effects. */
    FVM_API FVM_Result fvm_swf_stream_output_seek_frame(FVM_SWFStreamOutput *, unsigned int frame);
    /* At exhausted source (no pending PCM), bind a distinct valid converter with identical output
     * geometry. Preserve FIFO, pause, pending-buffer ownership and cumulative dequeue clock;
     * clear eof only. No callbacks/allocation/VM. INVALID_ARGUMENT for unfinished source,
     * reentry, null/self or mismatched geometry; other validation errors propagate unchanged.
     * Host owns ordering and exclusively retains both converters/snapshots until quiescence. */
    FVM_API FVM_Result fvm_swf_stream_output_continue(FVM_SWFStreamOutput *,
                                                       FVM_SWFStreamConverter *next);
    /* Explicit discontinuity to a converter/frame: validate geometry and binary frame selection,
     * then pause/clear and publish new binding/position only on clear success. Reset clock/phase,
     * discard pending, leave paused. Old converter unchanged when distinct; failed clear retains
     * old binding/pending and target cursor (acknowledged pause remains visible). No allocation/VM.
     * Caller exclusively retains target/snapshot; positive frame; same target is allowed. */
    FVM_API FVM_Result fvm_swf_stream_output_bind_frame(FVM_SWFStreamOutput *,
                                                         FVM_SWFStreamConverter *, unsigned int);
    /* Resolve authored MP3 timing before the existing pause/clear/seek transaction.
     * Same ownership, callback serialization, exception and failure contract as bind_frame.
     * Validate target geometry and signed position before callbacks; publish target/cursor only
     * after successful clear. Successful pause remains visible if clear fails. Success leaves
     * output paused, discards pending PCM, resets accounting and converter phase to zero.
     * Source snapshot must remain alive. No allocation, decoding, VM/GC or script execution;
     * backend callbacks run synchronously, reentry is rejected. Invalid frame/target/position
     * INVALID_ARGUMENT; encoded source UNSUPPORTED; overflow OUT_OF_MEMORY; callback statuses
     * propagate, callback exception ERROR. Failure preserves cursor/binding/pending PCM.
     * Host must authorize the selected authored frame before pumping resolved PCM; this does
     * not implement a full A/V clock or future-frame read-ahead policy. Legacy APIs unchanged. */
    FVM_API FVM_Result fvm_swf_stream_output_bind_frame_aligned(
        FVM_SWFStreamOutput *, FVM_SWFStreamConverter *, unsigned int frame);
    /* Current-source counterpart; same transactional and source-authorization contract. */
    FVM_API FVM_Result fvm_swf_stream_output_seek_frame_aligned(
        FVM_SWFStreamOutput *, unsigned int frame);
#ifdef __cplusplus
}
#endif
#endif
