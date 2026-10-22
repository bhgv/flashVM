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
        size_t fixed_bytes; /* Native owner including its two vector descriptors. */
        size_t buffer_bytes; /* Original and mixed fixed S16 arrays. */
        size_t retained_bytes; /* Checked sum of fixed and buffer bytes. */
    } FVM_SWFSoundMixQueueStorageInfo;
    /* Predict exact requested adapter bytes without allocating or calling VM/backend/mixer.
     * channels 1/2 and block 1..OUTPUT_MAX_BLOCK required; invalid -> INVALID_ARGUMENT,
     * size overflow -> OUT_OF_MEMORY. Required scalar out is preserved on errors. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_storage_required(unsigned int channels,
                                                                size_t block, size_t *out);
    /* Copy fixed storage observation, independent of pending/accepted queue frames.
     * Live serialized queue and exact-size out required; invalid/reentry preserves out.
     * No allocation/callback/VM/GC. Counts allocation requests, not actual RSS. */
    FVM_API FVM_Result fvm_swf_sound_mix_queue_storage_info(const FVM_SWFSoundMixQueue *,
                                                            FVM_SWFSoundMixQueueStorageInfo *);
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
