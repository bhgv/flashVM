/* Native PCM production is independent of slow VM/render/presentation work on the main thread. */
#include "audio.h"
#include "flashvm/movieclip.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

/* Include the retained segment table before preparing any individual snapshot. */
static FVM_Result stream_budget(size_t streams, size_t *remaining)
{
    size_t budget = FVM_DESKTOP_AUDIO_PREPARED_BYTES;
    if (streams > budget / sizeof(FVM_DesktopStreamSegment))
        return FVM_OUT_OF_MEMORY;
    *remaining = budget - streams * sizeof(FVM_DesktopStreamSegment);
    return FVM_OK;
}

/* Predict the separate composition ceiling before native endpoint or callback acquisition. */
static FVM_Result composition_budget(size_t streams, size_t events, size_t *out)
{
    FVM_Result result;
    *out = 0;
    if (!streams || !events) return FVM_OK;
    result = fvm_swf_sound_mix_queue_storage_required_for_tracking(
        FVM_DESKTOP_AUDIO_CHANNELS, FVM_DESKTOP_AUDIO_BLOCK, 1, out);
    if (result != FVM_OK) return result;
    return *out <= FVM_DESKTOP_AUDIO_MIX_BYTES ? FVM_OK : FVM_OUT_OF_MEMORY;
}

/* Charge retained predecessors plus this snapshot's temporary peak before native allocation. */
static FVM_Result prepare_segment(FVM_Context *context, size_t index, size_t *remaining,
                                  FVM_SWFStreamSoundSnapshot **out)
{
    FVM_SWFStreamSoundStorageInfo storage;
    FVM_Result result =
        fvm_swf_stream_sound_snapshot_open_pcm_bounded(context, index, *remaining, out);
    if (result != FVM_OK)
        return result;
    storage.struct_size = sizeof(storage);
    result = fvm_swf_stream_sound_snapshot_storage_info(*out, &storage);
    if (result == FVM_OK && storage.retained_bytes <= *remaining)
        *remaining -= storage.retained_bytes;
    else
    {
        fvm_swf_stream_sound_snapshot_close(*out);
        *out = 0;
        if (result == FVM_OK)
            result = FVM_OUT_OF_MEMORY;
    }
    return result;
}

/* Reject unsupported combinations before a window or audio endpoint can be opened. */
static FVM_Result validate_headers(FVM_Context *context, int decode)
{
    size_t streams = 0, index;
    size_t remaining = 0;
    FVM_SWFStreamSoundInfo info;
    FVM_Result result = fvm_swf_stream_sound_count(context, &streams);
    if (result != FVM_OK || !streams)
        return result;
    if (streams > FVM_DESKTOP_AUDIO_MAX_SEGMENTS)
        return FVM_UNSUPPORTED;
    result = stream_budget(streams, &remaining);
    if (result != FVM_OK)
        return result;
    for (index = 0; index < streams; ++index)
    {
        info.struct_size = sizeof(info);
        result = fvm_swf_stream_sound_at(context, index, &info);
        if (result != FVM_OK)
            return result;
        if (info.format != FVM_SWF_SOUND_PCM_NATIVE && info.format != FVM_SWF_SOUND_PCM_LE &&
            info.format != FVM_SWF_SOUND_ADPCM && info.format != FVM_SWF_SOUND_MP3)
            return FVM_UNSUPPORTED;
        if (decode)
        {
            FVM_SWFStreamSoundSnapshot *prepared = 0;
            result = prepare_segment(context, index, &remaining, &prepared);
            fvm_swf_stream_sound_snapshot_close(prepared);
            if (result != FVM_OK)
                return result;
        }
    }
    return FVM_OK;
}

/* Validate every referenced root MP3 once before device acquisition; fixed bitmap avoids repeated decode. */
static FVM_Result validate_mp3_events(FVM_Context *context)
{
    unsigned char checked[FVM_DESKTOP_AUDIO_CHECKED_BYTES];
    size_t count = 0, ordinal;
    FVM_Result result = fvm_swf_sound_event_count(context, &count);
    if (result != FVM_OK)
        return result;
    memset(checked, 0, sizeof(checked));
    for (ordinal = 0; ordinal < count; ++ordinal)
    {
        FVM_SWFSoundEvent event;
        FVM_SWFSoundInfo info;
        unsigned int mask;
        size_t byte, written = 0;
        event.struct_size = sizeof(event);
        result = fvm_swf_sound_event_at(context, ordinal, &event);
        if (result != FVM_OK)
            return result;
        if (event.flags & FVM_SWF_SOUND_EVENT_STOP)
            continue;
        if (event.sound_id >= FVM_DESKTOP_AUDIO_RESOURCE_IDS)
            return FVM_INVALID_ARGUMENT;
        byte = event.sound_id >> FVM_DESKTOP_AUDIO_ID_SHIFT;
        mask = 1U << (event.sound_id & FVM_DESKTOP_AUDIO_ID_MASK);
        if (checked[byte] & mask)
            continue;
        info.struct_size = sizeof(info);
        result = fvm_swf_sound_info(context, event.sound_id, &info);
        if (result != FVM_OK)
            return result;
        if (info.format == FVM_SWF_SOUND_MP3)
        {
            /* A zero-length read still validates every packet and writes no retained PCM. */
            result = fvm_swf_sound_read_pcm(context, event.sound_id, 0, 0, 0, 0, &written);
            if (result != FVM_OK)
                return result;
        }
        checked[byte] |= (unsigned char)mask;
    }
    return FVM_OK;
}

/* Validate aggregate preparation before window/device acquisition; no snapshot is retained. */
FVM_Result fvm_desktop_audio_validate(FVM_Context *context)
{
    size_t streams = 0, events = 0, bytes = 0;
    FVM_Result result = validate_headers(context, 1);
    if (result == FVM_OK) result = validate_mp3_events(context);
    if (result == FVM_OK) result = fvm_swf_stream_sound_count(context, &streams);
    if (result == FVM_OK) result = fvm_swf_sound_event_count(context, &events);
    if (result == FVM_OK) result = composition_budget(streams, events, &bytes);
    return result;
}

/* Forward the actual write, recording only blocks accepted by the real SDL adapter. */
static FVM_Result traced_write(void *raw, const short *pcm, size_t frames)
{
    FVM_DesktopAudio *audio = (FVM_DesktopAudio *)raw;
    FVM_Result result = audio->queue_backend.write(audio->queue_backend.userdata, pcm, frames);
    fvm_desktop_trace_event(audio->trace, "write", (unsigned long)frames, result);
    if (result == FVM_OK)
        fvm_desktop_trace_pcm(audio->trace, pcm, frames);
    return result;
}

/* Preserve the real software queue observation without changing its clock. */
static FVM_Result traced_queued(void *raw, size_t *frames)
{
    FVM_DesktopAudio *audio = (FVM_DesktopAudio *)raw;
    return audio->queue_backend.queued(audio->queue_backend.userdata, frames);
}

/* Record pause calls in the same serialized order as real backend operations. */
static FVM_Result traced_pause(void *raw, int paused)
{
    FVM_DesktopAudio *audio = (FVM_DesktopAudio *)raw;
    FVM_Result result = audio->queue_backend.pause(audio->queue_backend.userdata, paused);
    fvm_desktop_trace_event(audio->trace, "pause", (unsigned long)paused, result);
    return result;
}

/* Destructive clear is an evidence marker, never a rewind of the submitted PCM snapshot. */
static FVM_Result traced_clear(void *raw)
{
    FVM_DesktopAudio *audio = (FVM_DesktopAudio *)raw;
    FVM_Result result = audio->queue_backend.clear(audio->queue_backend.userdata);
    fvm_desktop_trace_event(audio->trace, "clear", 0, result);
    return result;
}

/* Copy navigation while optionally withholding new frame authorization during event preparation. */
static FVM_Result sync_timeline(FVM_Context *, FVM_Object *, FVM_DesktopAudio *, int authorize);

/* [GC:] Resolve/copy on the context owner without holding the native producer's mutex.
 * Owner calls are serialized; the worker can only retire voices during preparation, so the
 * available-byte observation is conservative. Recheck actual adoption under the mutex. */
static FVM_Result sound_event(FVM_Context *context, FVM_Object *clip, size_t ordinal, void *raw)
{
    FVM_DesktopAudio *audio = (FVM_DesktopAudio *)raw;
    FVM_SWFSoundEvent event;
    FVM_SWFSoundMixerStorageInfo storage;
    FVM_SWFSoundVoice *voice = 0;
    FVM_Result result;
    size_t remaining = 0;
    int handled = 0, active = 0;
    if (audio->composition)
    {
        /* Navigation must clear the old epoch before this frame's first new event is adopted. */
        result = sync_timeline(context, clip, audio, 0);
        if (result != FVM_OK) return result;
    }
    event.struct_size = sizeof(event);
    result = fvm_swf_sound_event_at(context, ordinal, &event);
    if (result != FVM_OK)
        return result;
    if (SDL_LockMutex(audio->mutex) != 0)
        return FVM_ERROR;
    if (event.flags & FVM_SWF_SOUND_EVENT_STOP)
    {
        result = audio->composition
                     ? fvm_swf_sound_mix_queue_stop_id(audio->composition, event.sound_id)
                     : fvm_swf_sound_output_stop_id(audio->output, event.sound_id);
        handled = 1;
    }
    else if (event.flags & FVM_SWF_SOUND_EVENT_NO_MULTIPLE)
    {
        result = audio->composition
                     ? fvm_swf_sound_mix_queue_sound_active(audio->composition, event.sound_id, &active)
                     : fvm_swf_sound_output_sound_active(audio->output, event.sound_id, &active);
        handled = result != FVM_OK || active;
    }
    if (!handled && result == FVM_OK)
    {
        storage.struct_size = sizeof(storage);
        result = fvm_swf_sound_mixer_storage_info(audio->mixer, &storage);
        if (result == FVM_OK && storage.retained_bytes <= FVM_DESKTOP_AUDIO_EVENT_BYTES)
            remaining = FVM_DESKTOP_AUDIO_EVENT_BYTES - storage.retained_bytes;
        else if (result == FVM_OK)
            result = FVM_OUT_OF_MEMORY;
    }
    SDL_UnlockMutex(audio->mutex);
    if (!handled && result == FVM_OK)
        result = fvm_swf_sound_voice_open_bounded(context, ordinal, remaining, &voice);
    if (SDL_LockMutex(audio->mutex) != 0)
    {
        fvm_swf_sound_voice_close(voice);
        return FVM_ERROR;
    }
    if (!handled && result == FVM_OK)
        result = audio->composition
                     ? fvm_swf_sound_mix_queue_submit_prepared_bounded(
                           audio->composition, &voice, FVM_DESKTOP_AUDIO_EVENT_BYTES)
                     : fvm_swf_sound_output_submit_prepared_bounded(
                           audio->output, &voice, FVM_DESKTOP_AUDIO_EVENT_BYTES);
    fvm_desktop_trace_event(audio->trace, "event", (unsigned long)ordinal, result);
    if (result == FVM_OK)
        ++audio->events;
    SDL_UnlockMutex(audio->mutex);
    fvm_swf_sound_voice_close(voice); /* Rejected temporary ownership is released outside the lock. */
    return result;
}

/* Cancel prepared, pending and queued native state without racing the producer. */
static FVM_Result stop_context_audio(void *raw)
{
    FVM_DesktopAudio *audio = (FVM_DesktopAudio *)raw;
    FVM_Result result;
    if (SDL_LockMutex(audio->mutex) != 0)
        return FVM_ERROR;
    if (audio->stream_source)
    {
        result = fvm_swf_stream_output_seek(
            &audio->stream_output,
            audio->stream_segments[audio->stream_index].converter.block_count, 0);
        if (result == FVM_OK)
            audio->stream_cancelled = 1;
    }
    else
        result = fvm_swf_sound_output_stop_release(audio->output);
    if (result == FVM_OK)
    {
        ++audio->stops;
        audio->was_starved = 0;
        SDL_AtomicSet(&audio->error, FVM_OK);
    }
    SDL_UnlockMutex(audio->mutex);
    return result;
}

/* Caller holds the worker mutex; pending event PCM survives mixer retirement and write rejection. */
static FVM_Result mixed_work(FVM_DesktopAudio *audio, int *out)
{
    FVM_SWFSoundMixerInfo voices;
    FVM_SWFSoundMixQueuePending pending;
    FVM_Result result;
    *out = 0;
    if (!audio->composition) return FVM_OK;
    voices.struct_size = sizeof(voices);
    pending.struct_size = sizeof(pending);
    result = fvm_swf_sound_mixer_info(audio->mixer, &voices);
    if (result == FVM_OK) result = fvm_swf_sound_mix_queue_pending(audio->composition, &pending);
    if (result == FVM_OK) *out = voices.active_voices != 0 || pending.frames != 0;
    return result;
}

/* Caller holds mutex. Inspect current and reset-on-activation successor metadata only.
 * Future frames are never authorized here, and no converter is advanced by this query. */
static FVM_Result stream_authorized(FVM_DesktopAudio *audio, int *out)
{
    unsigned int frame = 0;
    size_t index;
    FVM_Result result;
    if (audio->composition && !audio->stream_frame)
    {
        *out = 0;
        return FVM_OK;
    }
    if (audio->stream_cancelled) return mixed_work(audio, out);
    result = fvm_swf_stream_output_next_frame(&audio->stream_output, &frame);
    if (result == FVM_OK)
    {
        if (frame <= audio->stream_frame) { *out = 1; return FVM_OK; }
        return mixed_work(audio, out);
    }
    if (result != FVM_NOT_FOUND)
        return result;
    for (index = audio->stream_index + 1; index < audio->stream_count; ++index)
    {
        FVM_SWFStreamSoundBlock block;
        block.struct_size = sizeof(block);
        result = fvm_swf_stream_sound_snapshot_block_at(audio->stream_segments[index].source,
                                                        0, &block);
        if (result == FVM_NOT_FOUND)
            continue;
        if (result != FVM_OK)
            return result;
        if (block.frame <= audio->stream_frame) { *out = 1; return FVM_OK; }
        return mixed_work(audio, out);
    }
    return mixed_work(audio, out);
}

/* Activate the next immutable source under the worker mutex without clearing either queue clock. */
static FVM_Result stream_continue_segment(FVM_DesktopAudio *audio)
{
    FVM_SWFStreamConverter *next = &audio->stream_segments[audio->stream_index + 1].converter;
    FVM_Result result = fvm_swf_stream_converter_seek(next, 0, 0);
    if (result == FVM_OK) result = fvm_swf_stream_output_continue(&audio->stream_output, next);
    if (result != FVM_OK) return result;
    ++audio->stream_index;
    fvm_desktop_trace_event(audio->trace, "stream_segment", (unsigned long)audio->stream_index, FVM_OK);
    return FVM_OK;
}

/* Caller holds mutex; one authorized base producer and optional EOF event tails share the FIFO. */
static FVM_Result refill_stream(FVM_DesktopAudio *audio)
{
    FVM_SWFStreamOutputInfo info;
    FVM_Result result;
    unsigned int attempt;
    int held = 0, authorized = 0;
    if (audio->stream_movie_paused || (audio->stream_cancelled && !audio->composition) ||
        (audio->composition && !audio->stream_frame))
    {
        audio->was_starved = 0;
        audio->stream_waiting = 1;
        return fvm_swf_stream_output_pause(&audio->stream_output, 1);
    }
    info.struct_size = sizeof(info);
    result = fvm_swf_stream_output_info(&audio->stream_output, &info);
    if (result != FVM_OK)
        return result;
    result = stream_authorized(audio, &authorized);
    if (result != FVM_OK)
        return result;
    /* Initial priming, timeline gates and final drain are not observed starvation.
     * Pause acknowledgement must succeed before changing counters or edge state. */
    if (!authorized)
    {
        audio->was_starved = 0;
        audio->stream_waiting = 1;
    }
    else if (!info.paused && !info.queued_frames && info.submitted_frames > 0 &&
             !audio->was_starved && !audio->stream_waiting)
    {
        result = fvm_swf_stream_output_pause(&audio->stream_output, 1);
        if (result != FVM_OK)
            return result;
        if (SDL_AtomicGet(&audio->underruns) < INT_MAX)
            SDL_AtomicAdd(&audio->underruns, 1);
        audio->was_starved = 1;
        fvm_desktop_trace_event(audio->trace, "stream_starved", 0, FVM_OK);
    }
    if (authorized)
        audio->stream_waiting = 0;
    for (attempt = 0; attempt < FVM_DESKTOP_AUDIO_PUMPS; ++attempt)
    {
        if (audio->composition)
        {
            FVM_SWFSoundMixQueuePending pending;
            pending.struct_size = sizeof(pending);
            result = fvm_swf_sound_mix_queue_pending(audio->composition, &pending);
            if (result != FVM_OK) return result;
            if (pending.origin != FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL &&
                !audio->stream_cancelled && audio->stream_index + 1 < audio->stream_count)
            {
                unsigned int next_frame = 0;
                result = fvm_swf_stream_output_next_frame(&audio->stream_output, &next_frame);
                if (result == FVM_NOT_FOUND)
                {
                    result = stream_continue_segment(audio);
                    if (result != FVM_OK) return result;
                    continue; /* Do not synthesize event-only time ahead of an authorized successor. */
                }
                if (result != FVM_OK) return result;
            }
            result = fvm_swf_stream_output_pump_mix_until(
                &audio->stream_output, audio->composition, audio->stream_frame, 0);
        }
        else
            result = fvm_swf_stream_output_pump_until(&audio->stream_output, audio->stream_frame, 0);
        if (result == FVM_NOT_FOUND)
        {
            info.struct_size = sizeof(info);
            result = fvm_swf_stream_output_info(&audio->stream_output, &info);
            if (result != FVM_OK)
                return result;
            if (info.eof && !audio->stream_cancelled && audio->stream_index + 1 < audio->stream_count)
            {
                result = stream_continue_segment(audio);
                if (result != FVM_OK) return result;
                continue; /* Count empty-header transitions within the same fixed work budget. */
            }
            held = 1;
            break;
        }
        if (result != FVM_OK)
            return result;
    }
    info.struct_size = sizeof(info);
    result = fvm_swf_stream_output_info(&audio->stream_output, &info);
    /* A short authorized block/EOF cannot reach a full driver reserve; let it drain. */
    if (result == FVM_OK && info.paused && info.queued_frames &&
        (held || info.queued_frames >= audio->resume_frames))
        result = fvm_swf_stream_output_pause(&audio->stream_output, 0);
    if (result == FVM_OK && audio->was_starved && info.queued_frames)
    {
        /* Recheck acknowledged pause after an attempted resume; errors remain observable. */
        result = fvm_swf_stream_output_info(&audio->stream_output, &info);
        if (result == FVM_OK && !info.paused)
        {
            audio->was_starved = 0;
            fvm_desktop_trace_event(audio->trace, "stream_recovered",
                                    (unsigned long)info.queued_frames, FVM_OK);
        }
    }
    if (result == FVM_OK)
    {
        result = stream_authorized(audio, &authorized);
        if (result == FVM_OK && !authorized)
            audio->stream_waiting = 1;
    }
    return result;
}

/* Caller holds the mutex; this path uses only independent native PCM and SDL queue state. */
static FVM_Result refill(FVM_DesktopAudio *audio)
{
    FVM_SWFSoundOutputInfo info;
    FVM_Result result;
    unsigned int attempt;
    if (audio->stream_source)
        return refill_stream(audio);
    info.struct_size = sizeof(info);
    result = fvm_swf_sound_output_info(audio->output, &info);
    if (result != FVM_OK)
        return result;
    if (info.starved && !audio->was_starved)
    {
        SDL_AtomicAdd(&audio->underruns, 1);
        fvm_desktop_trace_event(audio->trace, "starved", 0, FVM_OK);
    }
    audio->was_starved = info.starved;
    for (attempt = 0; attempt < FVM_DESKTOP_AUDIO_PUMPS; ++attempt)
    {
        result = fvm_swf_sound_output_pump(audio->output, 0);
        if (result == FVM_NOT_FOUND)
            break;
        if (result != FVM_OK)
            return result;
    }
    result = fvm_swf_sound_output_info(audio->output, &info);
    if (result == FVM_OK && info.paused && info.queued_frames >= audio->resume_frames)
        result = fvm_swf_sound_output_pause(audio->output, 0);
    return result;
}

/* No VM/GC pointer is read here: native snapshots and producer access are mutex-serialized. */
static int SDLCALL audio_worker(void *raw)
{
    FVM_DesktopAudio *audio = (FVM_DesktopAudio *)raw;
    while (SDL_AtomicGet(&audio->running))
    {
        FVM_Result result = FVM_OK;
        if (SDL_LockMutex(audio->mutex) != 0)
        {
            SDL_AtomicSet(&audio->error, FVM_ERROR);
            return 1;
        }
        if (!audio->suspended && SDL_AtomicGet(&audio->error) == FVM_OK)
            result = refill(audio);
        if (result != FVM_OK)
            SDL_AtomicSet(&audio->error, (int)result);
        SDL_UnlockMutex(audio->mutex);
        SDL_Delay(FVM_DESKTOP_AUDIO_WAIT);
    }
    return 0;
}

/* Complete acquisition before publication; partial cleanup joins any started worker first. */
FVM_Result fvm_desktop_audio_open_paused(FVM_Context *context, FVM_Object *clip,
                                         FVM_DesktopAudio *audio, int paused)
{
    FVM_AudioQueueBackend backend;
    FVM_SDLAudioQueueInfo device_info;
    FVM_SWFSoundMixerConfig geometry;
    FVM_Result result;
    size_t count = 0, streams = 0, mix_budget = 0;
    if (!audio || (paused != 0 && paused != 1))
        return FVM_INVALID_ARGUMENT;
    memset(audio, 0, sizeof(*audio));
    audio->suspended = paused; /* Publish the initial policy before the worker can prime/unpause. */
    result = fvm_swf_sound_event_count(context, &count);
    if (result != FVM_OK)
        return result;
    result = fvm_swf_stream_sound_count(context, &streams);
    if (result != FVM_OK || (!count && !streams))
        return result;
    result =
        validate_headers(context, 0); /* Preparation below validates ADPCM before device open. */
    if (result != FVM_OK)
        return result;
    if (streams)
    {
        size_t index, remaining = 0;
        result = stream_budget(streams, &remaining);
        if (result != FVM_OK)
            return result;
        audio->stream_segments =
            (FVM_DesktopStreamSegment *)calloc(streams, sizeof(*audio->stream_segments));
        if (!audio->stream_segments)
            return FVM_OUT_OF_MEMORY;
        audio->stream_count = streams;
        for (index = 0; index < streams; ++index)
        {
            FVM_DesktopStreamSegment *segment = &audio->stream_segments[index];
            FVM_SWFStreamSoundInfo info;
            info.struct_size = sizeof(info);
            result = fvm_swf_stream_sound_at(context, index, &info);
            if (result == FVM_OK)
                result = prepare_segment(context, index, &remaining, &segment->source);
            if (result != FVM_OK)
                return result;
            segment->frame = info.frame;
            if (!index)
                audio->stream_source = segment->source;
            result =
                fvm_swf_stream_converter_init_continuous(&segment->converter, segment->source,
                                              FVM_DESKTOP_AUDIO_RATE, FVM_DESKTOP_AUDIO_CHANNELS);
            if (result != FVM_OK)
                return result;
        }
    }
    if (!streams || count)
    {
        geometry.struct_size = sizeof(geometry);
        geometry.sample_rate = FVM_DESKTOP_AUDIO_RATE;
        geometry.channels = FVM_DESKTOP_AUDIO_CHANNELS;
        geometry.voice_capacity = FVM_DESKTOP_AUDIO_VOICES;
        geometry.flags = FVM_SWF_SOUND_MIXER_CONVERT | FVM_SWF_SOUND_MIXER_DECLICK;
        result = fvm_swf_sound_mixer_create_bounded(
            &geometry, FVM_DESKTOP_AUDIO_EVENT_BYTES, &audio->mixer);
        if (result != FVM_OK)
            return result;

    }
    result = composition_budget(streams, count, &mix_budget);
    if (result != FVM_OK) return result;
    result = fvm_audio_sdl_queue_open(FVM_DESKTOP_AUDIO_RATE, FVM_DESKTOP_AUDIO_CHANNELS,
                                      FVM_DESKTOP_AUDIO_BLOCK, &audio->device, &backend);
    if (result != FVM_OK)
        return result;
    device_info.struct_size = sizeof(device_info);
    result = fvm_audio_sdl_queue_info(audio->device, &device_info);
    if (result != FVM_OK)
        return result;
    audio->target_frames = (size_t)device_info.device_buffer_frames * FVM_DESKTOP_AUDIO_QUANTA;
    if (audio->target_frames < FVM_DESKTOP_AUDIO_MIN_TARGET)
        audio->target_frames = FVM_DESKTOP_AUDIO_MIN_TARGET;
    audio->resume_frames =
        (size_t)device_info.device_buffer_frames * FVM_DESKTOP_AUDIO_RESUME_QUANTA;
    if (audio->resume_frames < FVM_DESKTOP_AUDIO_BLOCK)
        audio->resume_frames = FVM_DESKTOP_AUDIO_BLOCK;
    audio->trace = fvm_desktop_trace_open(&device_info, audio->target_frames, audio->resume_frames);
    if (audio->trace)
    {
        audio->queue_backend = backend;
        backend.userdata = audio;
        backend.write = traced_write;
        backend.queued = traced_queued;
        backend.pause = traced_pause;
        backend.clear = traced_clear;
    }
    if (mix_budget)
    {
        FVM_AudioQueueBackend adapted;
        result = fvm_swf_sound_mix_queue_create_bounded(audio->mixer, &backend,
            FVM_DESKTOP_AUDIO_BLOCK, mix_budget, &audio->composition, &adapted);
        if (result == FVM_OK) result = fvm_swf_sound_mix_queue_clock_begin(audio->composition);
        if (result == FVM_OK) result = fvm_swf_sound_mix_queue_set_identity_tracking(
            audio->composition, 1, mix_budget);
        if (result != FVM_OK) return result;
        backend = adapted; /* Trace wraps the real device below the single saturation pass. */
    }
    if (audio->stream_source)
    {
        result = fvm_swf_stream_output_init(
            &audio->stream_output, &audio->stream_segments[0].converter, &backend,
            audio->stream_pcm, sizeof(audio->stream_pcm) / sizeof(short), FVM_DESKTOP_AUDIO_BLOCK,
            audio->target_frames);
        if (result != FVM_OK)
            return result;
    }
    else
    {
        result = fvm_swf_sound_output_create(audio->mixer, &backend, FVM_DESKTOP_AUDIO_BLOCK,
                                             audio->target_frames, &audio->output);
        if (result != FVM_OK)
            return result;
        result = fvm_swf_sound_output_set_identity_tracking(audio->output, 1);
        if (result != FVM_OK)
            return result;
        result = fvm_swf_sound_output_set_release(
            audio->output, FVM_DESKTOP_AUDIO_RATE / FVM_DESKTOP_AUDIO_RELEASE_DIVISOR);
        if (result != FVM_OK)
            return result;
    }
    audio->mutex = SDL_CreateMutex();
    if (!audio->mutex)
        return FVM_OUT_OF_MEMORY;
    result =
        fvm_audio_stop_binding_create(context, stop_context_audio, audio, &audio->stop_binding);
    if (result == FVM_OK && (!audio->stream_source || audio->composition))
        result = fvm_movieclip_set_sound_callback(context, sound_event, audio);
    if (result == FVM_OK)
        result = fvm_desktop_audio_timeline(context, clip, audio);
    if (result != FVM_OK)
        return result;
    SDL_AtomicSet(&audio->running, 1);
    audio->thread = SDL_CreateThread(audio_worker, "FlashVM PCM", audio);
    if (!audio->thread)
    {
        SDL_AtomicSet(&audio->running, 0);
        return FVM_ERROR;
    }
    SDL_Log("FlashVM audio: driver=%s quantum=%u target=%lu resume=%lu rate=%u channels=%u",
            SDL_GetCurrentAudioDriver(), device_info.device_buffer_frames,
            (unsigned long)audio->target_frames, (unsigned long)audio->resume_frames,
            device_info.sample_rate, device_info.channels);
    return FVM_OK;
}

/* Existing callers retain running startup and the same ownership/cleanup contract. */
FVM_Result fvm_desktop_audio_open(FVM_Context *context, FVM_Object *clip, FVM_DesktopAudio *audio)
{
    return fvm_desktop_audio_open_paused(context, clip, audio, 0);
}

/* Caller owns the mutex; last header at/before frame wins, or the first future header. */
static FVM_Result stream_seek_frame(FVM_DesktopAudio *audio, unsigned int frame)
{
    size_t index = 0;
    FVM_Result result;
    while (index + 1 < audio->stream_count && audio->stream_segments[index + 1].frame <= frame)
        ++index;
    result = fvm_swf_stream_output_bind_frame(&audio->stream_output,
                                              &audio->stream_segments[index].converter, frame);
    if (result == FVM_OK)
        audio->stream_index = index;
    return result;
}

/* [GC:] VM inspection stays on owner; native producer receives only copied scalar policy. */
static FVM_Result sync_timeline(FVM_Context *context, FVM_Object *clip,
                                 FVM_DesktopAudio *audio, int authorize)
{
    FVM_MovieClipPlaybackInfo info;
    FVM_Result result = FVM_OK;
    int reset;
    unsigned long navigation = 0;
    if (!audio)
        return FVM_INVALID_ARGUMENT;
    if (!audio->stream_source)
        return FVM_OK;
    info.struct_size = sizeof(info);
    result = fvm_movieclip_get_playback_info(context, clip, &info);
    if (result != FVM_OK)
        return result;
    result = fvm_movieclip_get_navigation(context, clip, &navigation);
    if (result != FVM_OK)
        return result;
    if (SDL_LockMutex(audio->mutex) != 0)
        return FVM_ERROR;
    reset = audio->stream_selected &&
            (navigation != audio->stream_navigation ||
             (!info.entered && (audio->stream_entered || info.frame != audio->stream_selected)) ||
             info.frame < audio->stream_selected || info.frame > audio->stream_selected + 1);
    if (reset)
    {
        result = stream_seek_frame(audio, info.frame);
        if (result == FVM_OK)
        {
            audio->stream_cancelled = 0;
            audio->was_starved = 0;
            audio->stream_waiting = 1;
            fvm_desktop_trace_event(audio->trace, "stream_seek", info.frame, result);
        }
    }
    if (result == FVM_OK)
    {
        audio->stream_navigation = navigation;
        audio->stream_selected = info.frame;
        audio->stream_entered = info.entered;
        audio->stream_frame = authorize && info.entered ? info.frame : 0;
        /* Frame events may allocate outside the mutex. The owner publishes permission only
         * after the complete frame, so the worker cannot render before all starts arrive. */
        audio->stream_movie_paused = !info.playing;
        if (!info.playing || !info.entered)
            result = fvm_swf_stream_output_pause(&audio->stream_output, 1);
    }
    SDL_UnlockMutex(audio->mutex);
    return result;
}

/* [GC:] Publish completed root frame authorization only from the VM owner's safe point. */
FVM_Result fvm_desktop_audio_timeline(FVM_Context *context, FVM_Object *clip,
                                      FVM_DesktopAudio *audio)
{
    return sync_timeline(context, clip, audio, 1);
}

/* Background suspension preserves the FIFO; worker primes again before foreground resume. */
FVM_Result fvm_desktop_audio_pause(FVM_DesktopAudio *audio, int paused)
{
    FVM_Result result = FVM_OK;
    if (paused != 0 && paused != 1)
        return FVM_INVALID_ARGUMENT;
    if (!audio->output && !audio->stream_source)
        return FVM_OK;
    if (SDL_LockMutex(audio->mutex) != 0)
        return FVM_ERROR;
    if (paused)
        result = audio->stream_source ? fvm_swf_stream_output_pause(&audio->stream_output, 1)
                                      : fvm_swf_sound_output_pause(audio->output, 1);
    if (result == FVM_OK)
    {
        audio->suspended = paused;
        if (paused)
        {
            audio->was_starved = 0;
            audio->stream_waiting = 1;
        }
    }
    SDL_UnlockMutex(audio->mutex);
    return result;
}

/* The render thread polls a latch; it no longer determines when PCM gets produced. */
FVM_Result fvm_desktop_audio_tick_paused(FVM_DesktopAudio *audio, int paused)
{
    FVM_Result result;
    if (!audio || (paused != 0 && paused != 1))
        return FVM_INVALID_ARGUMENT;
    if (!audio->output && !audio->stream_source)
        return FVM_OK;
    result = (FVM_Result)SDL_AtomicGet(&audio->error);
    if (result != FVM_OK)
        return result;
    return fvm_desktop_audio_pause(audio, paused);
}

/* Compatibility foreground poll preserves ordinary continuous playback behavior. */
FVM_Result fvm_desktop_audio_tick(FVM_DesktopAudio *audio)
{
    return fvm_desktop_audio_tick_paused(audio, 0);
}

/* Diagnostics share the producer lock, preventing simultaneous observe/pump operations. */
FVM_Result fvm_desktop_audio_info(FVM_DesktopAudio *audio, FVM_SWFSoundOutputInfo *info)
{
    FVM_Result result;
    if (!audio || !audio->output)
        return FVM_INVALID_ARGUMENT;
    if (SDL_LockMutex(audio->mutex) != 0)
        return FVM_ERROR;
    result = fvm_swf_sound_output_info(audio->output, info);
    SDL_UnlockMutex(audio->mutex);
    return result;
}

/* Copy stream diagnostics without racing native preparation or queue accounting. */
FVM_Result fvm_desktop_audio_stream_info(FVM_DesktopAudio *audio, FVM_SWFStreamOutputInfo *info)
{
    FVM_Result result;
    if (!audio || !audio->stream_source)
        return FVM_INVALID_ARGUMENT;
    if (SDL_LockMutex(audio->mutex) != 0)
        return FVM_ERROR;
    result = fvm_swf_stream_output_info(&audio->stream_output, info);
    if (result == FVM_OK && audio->stream_index + 1 < audio->stream_count)
        info->eof = 0; /* Source EOF is not complete sequence EOF while later headers remain. */
    SDL_UnlockMutex(audio->mutex);
    return result;
}

/* Observe bounded native work without pumping it on the owner or authorizing future movie frames.
 */
FVM_Result fvm_desktop_audio_stream_pending(FVM_DesktopAudio *audio, int *out)
{
    FVM_Result result = FVM_OK;
    FVM_SWFStreamOutputInfo info;
    int pending = 0, authorized = 0;
    if (!audio || !out)
        return FVM_INVALID_ARGUMENT;
    if (!audio->stream_source)
    {
        *out = 0;
        return FVM_OK;
    }
    if (SDL_LockMutex(audio->mutex) != 0)
        return FVM_ERROR;
    result = (FVM_Result)SDL_AtomicGet(&audio->error);
    if (result == FVM_OK && !audio->suspended && !audio->stream_movie_paused &&
        (!audio->stream_cancelled || audio->composition))
    {
        info.struct_size = sizeof(info);
        result = fvm_swf_stream_output_info(&audio->stream_output, &info);
        if (result == FVM_OK)
        {
            result = stream_authorized(audio, &authorized);
            pending = info.queued_frames != 0 || authorized;
        }
    }
    if (result == FVM_OK)
        *out = pending;
    SDL_UnlockMutex(audio->mutex);
    return result;
}

/* Root content removal must silence the old native producer without ending the VM/window.
 * Detach the VM callback first; close joins the native-only worker before freeing PCM ownership. */
FVM_Result fvm_desktop_audio_clear(FVM_Context *context, FVM_DesktopAudio *audio)
{
    FVM_Result result;
    unsigned long events, stops;
    int underruns;
    if (!context || !audio)
        return FVM_INVALID_ARGUMENT;
    result = fvm_movieclip_set_sound_callback(context, 0, 0);
    if (result != FVM_OK)
        return result;
    events = audio->events;
    stops = audio->stops;
    underruns = SDL_AtomicGet(&audio->underruns);
    fvm_desktop_audio_close(audio);
    /* Keep run diagnostics cumulative even though device ownership has ended. */
    audio->events = events;
    audio->stops = stops;
    SDL_AtomicSet(&audio->underruns, underruns);
    return FVM_OK;
}

/* Release context registration before userdata; quiesce the worker before native cleanup. */
void fvm_desktop_audio_close(FVM_DesktopAudio *audio)
{
    fvm_audio_stop_binding_close(audio->stop_binding);
    SDL_AtomicSet(&audio->running, 0);
    if (audio->thread)
        SDL_WaitThread(audio->thread, 0);
    if (audio->stream_source && audio->stream_output.struct_size)
        fvm_swf_stream_output_seek(
            &audio->stream_output,
            audio->stream_segments[audio->stream_index].converter.block_count, 0);
    if (audio->output)
        fvm_swf_sound_output_stop(audio->output);
    fvm_swf_sound_output_close(audio->output);
    fvm_swf_sound_mix_queue_close(audio->composition);
    fvm_swf_sound_mixer_close(audio->mixer);
    if (audio->stream_segments)
    {
        size_t index;
        for (index = 0; index < audio->stream_count; ++index)
            fvm_swf_stream_sound_snapshot_close(audio->stream_segments[index].source);
        free(audio->stream_segments);
    }
    fvm_desktop_trace_close(audio->trace);
    fvm_audio_sdl_queue_close(audio->device);
    if (audio->mutex)
        SDL_DestroyMutex(audio->mutex);
    memset(audio, 0, sizeof(*audio));
}
