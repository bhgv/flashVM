/* Native FIFO adapter and explicit file consumer; included after StreamFile/write_pcm/stop_pcm. */
#include "flashvm/swf_stream_output.h" /* Caller-buffered producer and native queue callback contract. */
typedef struct TutorialQueue { /* The host owns storage; no device or worker is hidden here. */
    short pcm[TUTORIAL_STREAM_QUEUE_FRAMES * TUTORIAL_STREAM_CHANNELS]; /* Accepted immutable FIFO data. */
    size_t frames; /* Complete output frames retained until the explicit consumer drains them. */
    int paused; /* Only an acknowledged resume permits the file consumer to advance. */
} TutorialQueue; /* The producer and consumer run serially in this small host. */
/* Accept all frames atomically into owned storage, never directly into a partially writable file. */
static FVM_Result queue_write(void *raw, const short *pcm, size_t frames) { /* Borrow input only during memcpy. */
    TutorialQueue *q = (TutorialQueue *)raw; /* Native stack state outlives all producer calls. */
    if (frames > TUTORIAL_STREAM_QUEUE_FRAMES - q->frames) return FVM_ERROR; /* Reject without touching FIFO. */
    memcpy(q->pcm + q->frames * TUTORIAL_STREAM_CHANNELS, pcm, frames * TUTORIAL_STREAM_CHANNELS * sizeof(short)); /* Own accepted bytes. */
    q->frames += frames; return FVM_OK; /* Publish acceptance only after the complete copy. */
} /* The real file side effect happens later in the separate consumer. */
/* Report actual retained native FIFO frames, including while paused. */
static FVM_Result queue_count(void *raw, size_t *frames) { /* The callback borrows only native state. */
    *frames = ((TutorialQueue *)raw)->frames; return FVM_OK; /* No sample-clock inference or I/O. */
} /* End exact backlog query. */
/* Change only the explicitly owned consumer pause state. */
static FVM_Result queue_pause(void *raw, int paused) { /* Producer supplies canonical zero/one. */
    ((TutorialQueue *)raw)->paused = paused; return FVM_OK; /* No implicit drain, clear or priming. */
} /* End pause acknowledgement. */
/* Discard only this host's owned FIFO, without closing the separately owned file. */
static FVM_Result queue_clear(void *raw) { /* Seek/reset may call this adapter. */
    ((TutorialQueue *)raw)->frames = 0; return FVM_OK; /* Old bytes become inaccessible, not externally replayed. */
} /* End bounded clear. */
/* The consumer owns irreversible file I/O; a partial write aborts, never enters producer retry. */
static FVM_Result queue_drain(TutorialQueue *queue, StreamFile *host, size_t *total) { /* Serial host consumer. */
    FVM_MediaFrame frame; FVM_Result result; /* Borrow retained FIFO bytes through synchronous write_pcm. */
    if (queue->paused) return FVM_ERROR; /* The example never consumes a paused queue. */
    memset(&frame, 0, sizeof(frame)); frame.kind = FVM_MEDIA_AUDIO; /* No movie/device clock is invented. */
    frame.data = (const unsigned char *)queue->pcm; frame.samples = queue->frames; /* Complete owned frames. */
    frame.size = queue->frames * TUTORIAL_STREAM_CHANNELS * sizeof(short); /* Exact initialized scalar extent. */
    frame.sample_rate = (unsigned int)TUTORIAL_STREAM_OUTPUT_RATE; frame.channels = TUTORIAL_STREAM_CHANNELS; /* Explicit output geometry. */
    result = write_pcm(host, &frame); if (result != FVM_OK) return result; /* Abort rather than repeat partial file bytes. */
    *total += queue->frames; queue->frames = 0; return FVM_OK; /* Publish full consumption only after successful write. */
} /* No file byte is handed back to the producer as an all-or-none acceptance claim. */
/* Prepare, enqueue and consume actual converted samples after source VM/runtime destruction. */
static FVM_Result export_queue(const FVM_SWFStreamSoundSnapshot *snapshot, StreamFile *host, size_t *total) { /* Synchronous host. */
    TutorialQueue queue; FVM_AudioQueueBackend backend; /* Host owns FIFO and callback userdata lifetime. */
    FVM_SWFStreamConverter converter; FVM_SWFStreamOutput output; /* Native position and serial producer. */
    FVM_SWFStreamOutputInfo info; FVM_SWFStreamSoundBlock block; /* Owned queue metadata and temporary source view. */
    short retry[TUTORIAL_STREAM_CHUNK_FRAMES * TUTORIAL_STREAM_CHANNELS]; /* Exclusive caller-owned pending buffer. */
    FVM_Result result; size_t accepted; /* Distinguish source markers, accepted frames and drained data. */
    memset(&queue, 0, sizeof(queue)); memset(&backend, 0, sizeof(backend)); /* No partial native handle ownership. */
    backend.struct_size = sizeof(backend); backend.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION; /* Exact callback ABI. */
    backend.sample_rate = (unsigned int)TUTORIAL_STREAM_OUTPUT_RATE; backend.channels = TUTORIAL_STREAM_CHANNELS; /* Match converter geometry. */
    backend.userdata = &queue; backend.write = queue_write; backend.queued = queue_count; /* All-or-none bounded native FIFO. */
    backend.pause = queue_pause; backend.clear = queue_clear; /* Explicit host operations, no hidden device. */
    result = fvm_swf_stream_converter_init(&converter, snapshot, TUTORIAL_STREAM_OUTPUT_RATE, TUTORIAL_STREAM_CHANNELS); /* Borrow native snapshot. */
    if (result != FVM_OK) return result; /* No file side effects before validated source geometry. */
    result = fvm_swf_stream_output_init(&output, &converter, &backend, retry, sizeof(retry) / sizeof(retry[0]), TUTORIAL_STREAM_CHUNK_FRAMES, TUTORIAL_STREAM_QUEUE_FRAMES); /* Empty queue becomes paused. */
    if (result != FVM_OK) return result; /* Caller still owns every object and buffer. */
    result = fvm_swf_stream_output_pause(&output, 0); if (result != FVM_OK) return result; /* Explicit consumer permission. */
    for (;;) { /* Producer work is bounded; full FIFO triggers explicit consumption. */
        result = fvm_swf_stream_output_pump(&output, &accepted); /* Retry storage remains exclusively producer-owned. */
        if (result != FVM_OK && result != FVM_NOT_FOUND) return result; /* Never retry irreversible file I/O. */
        info.struct_size = sizeof(info); /* Query observes actual retained FIFO and source EOF separately. */
        if (fvm_swf_stream_output_info(&output, &info) != FVM_OK) return FVM_ERROR; /* Reject a broken queue contract. */
        if (result == FVM_OK && (info.chunk.flags & FVM_STREAM_CONVERT_BEGIN)) { /* Report each newly prepared block once. */
            block.struct_size = sizeof(block); /* Use actual source length, not output frame count. */
            if (fvm_swf_stream_sound_snapshot_block_at(snapshot, info.chunk.block_index, &block) != FVM_OK) return FVM_ERROR; /* Borrow indexed metadata only. */
            printf("Frame %u: %lu sample frames\n", info.chunk.frame, (unsigned long)block.sample_count); /* Preserve gaps and empty blocks. */
        } /* A backpressure return must not print the previous block marker again. */
        if (queue.frames && (queue.frames == TUTORIAL_STREAM_QUEUE_FRAMES || info.eof)) { /* FIFO ownership moves through the explicit consumer. */
            result = queue_drain(&queue, host, total); if (result != FVM_OK) return result; /* File failure aborts this invocation. */
        } /* Subsequent info observes consumption through submitted-minus-queued. */
        if (info.eof && !queue.frames) break; /* EOF alone does not mean queued samples were consumed. */
    } /* All prepared/accepted samples have now reached the file adapter. */
    result = fvm_swf_stream_output_pause(&output, 1); if (result != FVM_OK) return result; /* Quiesce before native state leaves scope. */
    return stop_pcm(host); /* Flush and close the actual file after producer/consumer completion. */
} /* Main releases snapshot only after this borrowed converter/producer is quiesced. */
