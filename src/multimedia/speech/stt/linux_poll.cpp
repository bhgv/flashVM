/* OS: Linux with optional static ALSA/Vosk SDKs. Process a bounded actual microphone chunk. */
#include "linux_vosk.hpp" /* Import real native capture and recognizer interfaces. */
#if defined(__linux__) && !defined(__ANDROID__) && /* Require actual statically linked providers. */ \
    defined(FVM_HAS_LINUX_STT) /* Require actual statically linked providers. */
#include <errno.h> /* Recognize nonblocking PCM availability and interruption. */
#include <string.h> /* Copy recognized plain UTF-8 into host storage. */
/* Preserve pending actual text across queries instead of consuming more microphone input. */
FVM_Result fvm_linux_stt_poll(void *opaque, char *text, size_t capacity, size_t *required, /* Poll one bounded nonblocking capture/inference step. */
                              int *final) /* Poll one bounded nonblocking capture/inference step. */
{ /* Begin real native recognition polling. */
    FVM_LinuxStt *state = static_cast<FVM_LinuxStt *>(opaque); /* Recover explicit microphone/model ownership. */
    if (!state->pending) /* Read fresh audio only after an earlier transcript has been copied. */
    { /* Capture one bounded real PCM chunk. */
        snd_pcm_sframes_t frames = /* Read actual native-endian mono signed-16 microphone samples. */
            snd_pcm_readi(state->capture, state->samples, /* Read actual native-endian mono signed-16 microphone samples. */
                          FVM_VOSK_CAPTURE_FRAMES); /* Read actual native-endian mono signed-16 microphone samples. */
        if (frames == -EAGAIN || frames == -EINTR || frames == 0) /* Return control to host scheduling when no chunk is ready. */
            return FVM_NOT_FOUND; /* Return control to host scheduling when no chunk is ready. */
        if (frames < 0) /* Report overruns/suspension/device errors; host decides whether to close/reopen. */
            return FVM_ERROR; /* Report overruns/suspension/device errors; host decides whether to close/reopen. */
        int complete = vosk_recognizer_accept_waveform_s( /* Run real offline recognition on actual captured PCM. */
            state->recognizer, state->samples, (int)frames); /* Run real offline recognition on actual captured PCM. */
        if (complete < 0) /* Preserve actual model/inference failure. */
            return FVM_ERROR; /* Preserve actual model/inference failure. */
        state->final = complete ? 1 : 0; /* Pair utterance completion with the selected native result. */
        const char *json = complete ? vosk_recognizer_result(state->recognizer) /* Obtain actual final or partial recognition output. */
                                    : vosk_recognizer_partial_result( /* Obtain actual final or partial recognition output. */
                                          state->recognizer); /* Obtain actual final or partial recognition output. */
        FVM_Result result = fvm_vosk_text( /* Decode the configured result schema into plain scalar UTF-8. */
            json, complete != 0, &state->pending); /* Decode the configured result schema into plain scalar UTF-8. */
        if (result != FVM_OK) /* Preserve empty hypotheses and native parse/allocation errors. */
            return result; /* Preserve empty hypotheses and native parse/allocation errors. */
    } /* Finish real capture/inference step. */
    *required = strlen(state->pending) + 1; /* Count exact host text capacity including NUL. */
    *final = state->final; /* Preserve this transcript's own final/partial state. */
    if (!capacity) /* A query retains the exact recognized transcript. */
        return FVM_OK; /* A query retains the exact recognized transcript. */
    if (capacity < *required) /* Insufficient buffers do not consume recognized text. */
        return FVM_INVALID_ARGUMENT; /* Insufficient buffers do not consume recognized text. */
    memcpy(text, state->pending, *required); /* Copy complete actual UTF-8 text into caller-owned memory. */
    delete[] state->pending; /* Release only after successful host copying. */
    state->pending = 0; /* Permit the next bounded microphone/inference step. */
    return FVM_OK; /* Return recognized text, not its native JSON envelope. */
} /* Finish real native recognition polling. */
#endif /* Finish optional native implementation. */
