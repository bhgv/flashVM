/* OS: Linux with optional static ALSA/Vosk SDKs. Stop actual capture and release offline inference. */
#include "linux_vosk.hpp" /* Import native microphone/model ownership. */
#if defined(__linux__) && !defined(__ANDROID__) && defined(FVM_HAS_LINUX_STT) /* Isolate real SDK cleanup. */
/* Release complete or partially initialized native recognition state. */
void fvm_linux_stt_close(void *opaque) /* Close cannot return native cleanup errors through its void contract. */
{ /* Begin actual native cleanup. */
    FVM_LinuxStt *state = static_cast<FVM_LinuxStt *>(opaque); /* Recover owned real resources. */
    if (state->capture) /* Stop only an actually opened ALSA stream. */
    { /* Release microphone ownership before inference resources. */
        snd_pcm_drop(state->capture); /* Immediately stop/drop native capture instead of draining indefinitely. */
        snd_pcm_close(state->capture); /* Close the actual microphone handle. */
    } /* Finish real microphone cleanup. */
    if (state->recognizer) /* Release real recognizer inference state. */
        vosk_recognizer_free(state->recognizer); /* Release real recognizer inference state. */
    if (state->model) /* Release the actual offline model reference. */
        vosk_model_free(state->model); /* Release the actual offline model reference. */
    delete[] state->pending; /* Release any transcript retained after a capacity query. */
    delete state; /* Release unmanaged native ownership. */
} /* Finish real native cleanup. */
#endif /* Finish optional native implementation. */
