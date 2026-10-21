/* Caller-buffered prepared stream PCM production into an explicitly owned native FIFO backend. */
#ifndef FLASHVM_SWF_STREAM_OUTPUT_H
#define FLASHVM_SWF_STREAM_OUTPUT_H /* Guard the standalone C89 producer ABI. */
#include "swf_stream_convert.h"
#include "swf_sound_output.h"
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
    /* Acknowledge pause/resume without clearing, priming or advancing converter position. */
    FVM_API FVM_Result fvm_swf_stream_output_pause(FVM_SWFStreamOutput *, int paused);
    /* Observe backend backlog and copy clock/metadata; failures preserve caller output. Reject
     * impossible growth beyond submitted/target or regression of played_frames with ERROR.
     * Requires exclusive producer and queue ownership, including no independent clears/writes. */
    FVM_API FVM_Result fvm_swf_stream_output_info(FVM_SWFStreamOutput *, FVM_SWFStreamOutputInfo *);
    /* Validate absolute source seek first, pause then clear FIFO, discard pending and reset clock
     * epoch only after successful clear. New grid phase zero. Pause success remains visible if
     * clear fails; pending/converter/epoch stay intact for retry. Errors cannot undo host effects.
     * Leave queue paused; caller explicitly primes/resumes. EOF seek supported. */
    FVM_API FVM_Result fvm_swf_stream_output_seek(FVM_SWFStreamOutput *, size_t block,
                                                  size_t first);
#ifdef __cplusplus
}
#endif
#endif
