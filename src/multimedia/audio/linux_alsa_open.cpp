/* OS: Linux ALSA. Opening is explicit; disabled/non-Linux builds expose UNSUPPORTED. */
#include "linux_alsa.hpp" /* Import the optional native device layout. */
#include <new> /* Translate native wrapper allocation failures at the C boundary. */
#include <cstring> /* Clear a failed output table deterministically. */
/* Open the named PCM device in nonblocking playback mode and configure exact S16 stream geometry. */
FVM_Result fvm_audio_linux_alsa_open(const char *device, unsigned int rate, unsigned int channels, unsigned int latency_us, FVM_AlsaOutput **out, FVM_MediaOutput *output) /* Borrow the device name only during opening. */
{ /* Begin explicit native device acquisition. */
    if (out) /* Clear any valid handle destination before validating other arguments. */
        *out = 0; /* Never publish a partial device. */
    if (output) /* Clear any valid callback destination on every failure. */
        std::memset(output, 0, sizeof(*output)); /* Do not leave callable stale userdata after failure. */
    if (!out || !output || !device || !*device || !rate || !channels || !latency_us) /* Require explicit device and nonzero stream settings. */
        return FVM_INVALID_ARGUMENT; /* Avoid opening a device for malformed requests. */
#if defined(FVM_HAS_LINUX_ALSA_AUDIO) /* Use only the configured matching static SDK. */
    FVM_AlsaOutput *state = 0; /* Track private device ownership before publication. */
    try /* Contain wrapper allocation failure at the C ABI. */
    { /* Allocate state before acquiring the native resource. */
        state = new FVM_AlsaOutput; /* Allocate unmanaged host output state. */
    } /* Finish native wrapper allocation. */
    catch (const std::bad_alloc &) /* Translate allocation failure without touching ALSA. */
    { /* Preserve deterministic cleared outputs. */
        return FVM_OUT_OF_MEMORY; /* No host resource was opened. */
    } /* Finish allocation failure handling. */
    state->pcm = 0; /* Make partial-open cleanup deterministic. */
    state->rate = rate; /* Save the required sample rate for frame validation. */
    state->channels = channels; /* Save the required interleaved geometry. */
    state->prepared = false; /* Publish prepared state only after full configuration. */
    if (snd_pcm_open(&state->pcm, device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) /* Open the explicitly named real ALSA endpoint. */
    { /* Undo wrapper ownership after native open failure. */
        delete state; /* ALSA owns failed-open cleanup; no successful handle is published. */
        return FVM_ERROR; /* Return an ordinary device/configuration error. */
    } /* Finish native open failure handling. */
    if (snd_pcm_set_params(state->pcm, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate, 0, latency_us) < 0) /* Disable implicit software resampling. */
    { /* Clean a device whose complete configuration failed. */
        snd_pcm_close(state->pcm); /* Release the successfully opened native handle. */
        delete state; /* Release its unpublished wrapper. */
        return FVM_ERROR; /* Preserve cleared output destinations. */
    } /* Finish configuration failure cleanup. */
    state->prepared = true; /* set_params prepares the configured PCM stream. */
    output->struct_size = sizeof(*output); /* Publish the complete host callback ABI. */
    output->userdata = state; /* Borrow this device state until explicit provider close. */
    output->write = fvm_alsa_output_write; /* Submit frames through real snd_pcm_writei. */
    output->stop = fvm_alsa_output_stop; /* Stop through real snd_pcm_drop. */
    *out = state; /* Transfer device-wrapper ownership to the host caller. */
    return FVM_OK; /* No samples have been submitted yet. */
#else /* Keep default and foreign builds free of implicit ALSA dependencies. */
    return FVM_UNSUPPORTED; /* The host may select another output bridge explicitly. */
#endif /* Finish optional native implementation. */
} /* Finish explicit Linux PCM opening. */
