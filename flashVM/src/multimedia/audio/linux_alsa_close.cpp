/* OS: Linux/static ALSA. The host closes this provider only after unregistering dependent channels. */
#include "linux_alsa.hpp" /* Import native provider ownership. */
/* Drop pending output and close the device, invalidating the provider even on an I/O error. */
FVM_Result fvm_audio_linux_alsa_close(FVM_AlsaOutput *state) /* Null is safe for partial-setup cleanup. */
{ /* Begin terminal native device cleanup. */
    if (!state) /* Permit cleanup after unsupported or failed opening. */
        return FVM_OK; /* There is no device to release. */
#if defined(FVM_HAS_LINUX_ALSA_AUDIO) /* Close only handles created by the matching native implementation. */
    snd_pcm_drop(state->pcm); /* Best-effort discard before closing the underlying device. */
    int result = snd_pcm_close(state->pcm); /* Release ALSA's native resource regardless of stop state. */
    delete state; /* Invalidate provider userdata; no channel may still reference it. */
    return result < 0 ? FVM_ERROR : FVM_OK; /* Report close status without leaving a retryable wrapper. */
#else /* A valid non-null provider cannot have been created in a disabled build. */
    return FVM_UNSUPPORTED; /* Do not delete an unknown foreign pointer. */
#endif /* Finish optional native ownership release. */
} /* Finish terminal native provider cleanup. */
