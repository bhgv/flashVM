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
