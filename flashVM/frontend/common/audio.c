/* Native PCM production is independent of slow VM/render/presentation work on the main thread. */
#include "audio.h"
#include "flashvm/movieclip.h"
#include <string.h>

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

/* Synchronous frame-entry callback: source VM inspection always stays on its owning thread. */
static FVM_Result sound_event(FVM_Context *context, FVM_Object *clip, size_t ordinal, void *raw)
{
    FVM_DesktopAudio *audio = (FVM_DesktopAudio *)raw;
    FVM_Result result;
    (void)clip;
    if (SDL_LockMutex(audio->mutex) != 0)
        return FVM_ERROR;
    result = fvm_swf_sound_mixer_submit(audio->mixer, context, ordinal);
    fvm_desktop_trace_event(audio->trace, "event", (unsigned long)ordinal, result);
    if (result == FVM_OK)
        ++audio->events;
    SDL_UnlockMutex(audio->mutex);
    return result;
}

/* Cancel prepared, pending and queued native state without racing the producer. */
static FVM_Result stop_context_audio(void *raw)
{
    FVM_DesktopAudio *audio = (FVM_DesktopAudio *)raw;
    FVM_Result result;
    if (SDL_LockMutex(audio->mutex) != 0)
        return FVM_ERROR;
    result = fvm_swf_sound_output_stop(audio->output);
    if (result == FVM_OK)
    {
        ++audio->stops;
        audio->was_starved = 0;
        SDL_AtomicSet(&audio->error, FVM_OK);
    }
    SDL_UnlockMutex(audio->mutex);
    return result;
}

/* Caller holds the mutex; this path uses only independent native PCM and SDL queue state. */
static FVM_Result refill(FVM_DesktopAudio *audio)
{
    FVM_SWFSoundOutputInfo info;
    FVM_Result result;
    unsigned int attempt;
    info.struct_size = sizeof(info);
    result = fvm_swf_sound_output_info(audio->output, &info);
    if (result != FVM_OK)
        return result;
    if (info.starved && !audio->was_starved)
        SDL_AtomicAdd(&audio->underruns, 1);
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
FVM_Result fvm_desktop_audio_open(FVM_Context *context, FVM_Object *clip, FVM_DesktopAudio *audio)
{
    FVM_AudioQueueBackend backend;
    FVM_SDLAudioQueueInfo device_info;
    FVM_SWFSoundMixerConfig geometry;
    FVM_Result result;
    size_t count = 0;
    (void)clip;
    memset(audio, 0, sizeof(*audio));
    result = fvm_swf_sound_event_count(context, &count);
    if (result != FVM_OK || !count)
        return result;
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
    geometry.struct_size = sizeof(geometry);
    geometry.sample_rate = FVM_DESKTOP_AUDIO_RATE;
    geometry.channels = FVM_DESKTOP_AUDIO_CHANNELS;
    geometry.voice_capacity = FVM_DESKTOP_AUDIO_VOICES;
    geometry.flags = FVM_SWF_SOUND_MIXER_CONVERT;
    result = fvm_swf_sound_mixer_create_configured(&geometry, &audio->mixer);
    if (result != FVM_OK)
        return result;
    result = fvm_swf_sound_output_create(audio->mixer, &backend, FVM_DESKTOP_AUDIO_BLOCK,
                                         audio->target_frames, &audio->output);
    if (result != FVM_OK)
        return result;
    audio->mutex = SDL_CreateMutex();
    if (!audio->mutex)
        return FVM_OUT_OF_MEMORY;
    result =
        fvm_audio_stop_binding_create(context, stop_context_audio, audio, &audio->stop_binding);
    if (result == FVM_OK)
        result = fvm_movieclip_set_sound_callback(context, sound_event, audio);
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

/* Background suspension preserves the FIFO; worker primes again before foreground resume. */
FVM_Result fvm_desktop_audio_pause(FVM_DesktopAudio *audio, int paused)
{
    FVM_Result result = FVM_OK;
    if (paused != 0 && paused != 1)
        return FVM_INVALID_ARGUMENT;
    if (!audio->output)
        return FVM_OK;
    if (SDL_LockMutex(audio->mutex) != 0)
        return FVM_ERROR;
    if (paused)
        result = fvm_swf_sound_output_pause(audio->output, 1);
    if (result == FVM_OK)
        audio->suspended = paused;
    SDL_UnlockMutex(audio->mutex);
    return result;
}

/* The render thread polls a latch; it no longer determines when PCM gets produced. */
FVM_Result fvm_desktop_audio_tick(FVM_DesktopAudio *audio)
{
    FVM_Result result;
    if (!audio->output)
        return FVM_OK;
    result = (FVM_Result)SDL_AtomicGet(&audio->error);
    if (result != FVM_OK)
        return result;
    return fvm_desktop_audio_pause(audio, 0);
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

/* Release context registration before userdata; quiesce the worker before native cleanup. */
void fvm_desktop_audio_close(FVM_DesktopAudio *audio)
{
    fvm_audio_stop_binding_close(audio->stop_binding);
    SDL_AtomicSet(&audio->running, 0);
    if (audio->thread)
        SDL_WaitThread(audio->thread, 0);
    if (audio->output)
        fvm_swf_sound_output_stop(audio->output);
    fvm_swf_sound_output_close(audio->output);
    fvm_swf_sound_mixer_close(audio->mixer);
    fvm_desktop_trace_close(audio->trace);
    fvm_audio_sdl_queue_close(audio->device);
    if (audio->mutex)
        SDL_DestroyMutex(audio->mutex);
    memset(audio, 0, sizeof(*audio));
}
