/* OS: Linux/static ALSA. Stop immediately without draining or closing the device. */
#include "linux_alsa.hpp" /* Import the host-owned PCM handle. */
#if defined(FVM_HAS_LINUX_ALSA_AUDIO) /* Preserve dependency-free stubs in default builds. */
/* Discard pending device samples and require reprepare before subsequent output. */
FVM_Result fvm_alsa_output_stop(void *raw) /* This callback may be used by context StopSounds. */
{ /* Begin immediate native stopping. */
    FVM_AlsaOutput *state = static_cast<FVM_AlsaOutput *>(raw); /* Recover the still-live provider state. */
    if (!state) /* Reject absent host userdata. */
        return FVM_INVALID_ARGUMENT; /* Avoid entering the native library with no PCM handle. */
    if (snd_pcm_state(state->pcm) == SND_PCM_STATE_SETUP) /* A prior failed write may already have dropped the device queue. */
    { /* Acknowledge the already stopped endpoint without a redundant invalid drop. */
        state->prepared = false; /* Preserve the requirement to prepare before future output. */
        return FVM_OK; /* Treat an already discarded queue as successfully stopped. */
    } /* Finish idempotent stopped-state handling. */
    int result = snd_pcm_drop(state->pcm); /* Stop and discard queued PCM through the real ALSA API. */
    state->prepared = false; /* The next write must prepare even after a failed drop. */
    return result < 0 ? FVM_ERROR : FVM_OK; /* Preserve a retryable channel stop error. */
} /* Finish immediate native stopping. */
#endif /* Finish native-only callback implementation. */
