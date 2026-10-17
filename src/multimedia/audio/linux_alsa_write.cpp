/* OS: Linux/static ALSA. Complete-frame submission never waits for buffer space. */
#include "linux_alsa.hpp" /* Import native device state and frame validation. */
#include <limits> /* Bound the conversion to ALSA's frame-count type. */
#if defined(FVM_HAS_LINUX_ALSA_AUDIO) /* Compile actual library calls only with the explicit SDK. */
/* Fail with queued data discarded if the endpoint cannot accept the complete frame immediately. */
FVM_Result fvm_alsa_output_write(void *raw, const FVM_MediaFrame *frame) /* Borrow PCM data for this synchronous callback only. */
{ /* Begin bounded native PCM submission. */
    FVM_AlsaOutput *state = static_cast<FVM_AlsaOutput *>(raw); /* Recover live host-owned device state. */
    if (!state || !fvm_media_frame_valid(frame, FVM_MEDIA_AUDIO) || frame->sample_rate != state->rate || frame->channels != state->channels || frame->samples > std::numeric_limits<snd_pcm_uframes_t>::max()) /* Validate geometry before any device operation. */
        return FVM_INVALID_ARGUMENT; /* Leave existing device state unchanged on malformed input. */
    if (!state->prepared && snd_pcm_prepare(state->pcm) < 0) /* Reprepare after a prior explicit drop or output failure. */
        return FVM_ERROR; /* No new data was submitted. */
    state->prepared = true; /* This stream can now accept interleaved sample frames. */
    size_t consumed = 0; /* Count accepted sample frames rather than bytes. */
    while (consumed < frame->samples) /* Every successful iteration consumes at least one frame. */
    { /* Submit the remaining borrowed PCM region. */
        const unsigned char *data = frame->data + consumed * state->channels * sizeof(short); /* Advance across native S16 interleaved samples. */
        snd_pcm_sframes_t count = snd_pcm_writei(state->pcm, data, static_cast<snd_pcm_uframes_t>(frame->samples - consumed)); /* Make a nonblocking real ALSA write. */
        if (count <= 0 || static_cast<size_t>(count) > frame->samples - consumed) /* Treat EAGAIN/underrun/zero progress as an explicit failed complete-frame write. */
        { /* Clear pending native output before reporting failed submission. */
            snd_pcm_drop(state->pcm); /* Discard queued data without an implicit wait or retry loop. */
            state->prepared = false; /* Force explicit reprepare on the next submission. */
            return FVM_ERROR; /* The host may retry a frame according to its own clock policy. */
        } /* Finish failed partial submission cleanup. */
        consumed += static_cast<size_t>(count); /* Bound total progress by the validated frame count. */
    } /* Finish complete borrowed-frame consumption. */
    if (snd_pcm_state(state->pcm) == SND_PCM_STATE_PREPARED && snd_pcm_start(state->pcm) < 0) /* Start short accepted frames without waiting for a buffer threshold. */
    { /* Clear pending samples if explicit playback start failed. */
        snd_pcm_drop(state->pcm); /* Discard output rather than reporting successful silent queuing. */
        state->prepared = false; /* Reprepare before a later write attempt. */
        return FVM_ERROR; /* Preserve explicit device-start failure. */
    } /* Finish explicit short-frame start handling. */
    return FVM_OK; /* The entire frame was accepted; the context channel may become active. */
} /* Finish nonblocking native output. */
#endif /* Finish optional native library calls. */
