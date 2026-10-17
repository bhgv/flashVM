/* OS: Linux with optional static ALSA/Vosk SDKs. Open a real explicit microphone and offline model. */
#include "linux_vosk.hpp" /* Import actual native library ownership. */
#if defined(__linux__) && !defined(__ANDROID__) && /* Require native SDKs selected by CMake. */ \
    defined(FVM_HAS_LINUX_STT) /* Require native SDKs selected by CMake. */
#include <limits.h> /* Verify the native signed-16 representation accepted by Vosk. */
#include <new> /* Allocate native state without exceptions or STL. */
#include <string.h> /* Initialize all partial ownership pointers. */
/* Load the actual model before activating capture; cleanup owns every partial allocation. */
FVM_Result fvm_linux_stt_open(void *opaque, void **out) /* The host has explicitly authorized this capture operation. */
{ /* Begin real offline recognition startup. */
    const FVM_LinuxSttConfig *config = /* Borrow explicit model/device configuration. */
        static_cast<const FVM_LinuxSttConfig *>(opaque); /* Borrow explicit model/device configuration. */
    *out = 0; /* Publish no partial recognizer or microphone. */
    if (sizeof(short) != 2 || SHRT_MAX != 32767) /* Require actual native signed-16 samples. */
        return FVM_UNSUPPORTED; /* Require actual native signed-16 samples. */
    FVM_LinuxStt *state = new (std::nothrow) FVM_LinuxStt; /* Allocate unmanaged native ownership. */
    if (!state) /* Do not open model/capture after allocation failure. */
        return FVM_OUT_OF_MEMORY; /* Do not open model/capture after allocation failure. */
    memset(state, 0, sizeof(*state)); /* Make partial-failure cleanup safe. */
    state->model = vosk_model_new( /* Load the actual installed offline recognition model; never download it. */
        config->model_path); /* Load the actual installed offline recognition model; never download it. */
    if (state->model) /* Create the real model-backed speech recognizer. */
        state->recognizer = vosk_recognizer_new( /* Create the real model-backed speech recognizer. */
            state->model, (float)FVM_VOSK_SAMPLE_RATE); /* Create the real model-backed speech recognizer. */
    if (!state->model || !state->recognizer) /* Preserve missing/corrupt model or allocation failure. */
    { /* Release successfully created native siblings. */
        fvm_linux_stt_close(state); /* Free model/recognizer state without an active microphone. */
        return FVM_ERROR; /* Do not present absent model inference as valid recognition. */
    } /* Finish native model setup failure. */
    vosk_recognizer_set_max_alternatives(state->recognizer, 0); /* Keep the exact one-text JSON result schema. */
    vosk_recognizer_set_words(state->recognizer, 0); /* Avoid extra word/timestamp object fields. */
    if (snd_pcm_open(&state->capture, config->alsa_device, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK) < 0 || /* Explicitly open, configure and activate the actual ALSA microphone. */
        snd_pcm_set_params(state->capture, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, 1, FVM_VOSK_SAMPLE_RATE, /* Explicitly open, configure and activate the actual ALSA microphone. */
                           1, FVM_VOSK_LATENCY_MICROSECONDS) < 0 || /* Explicitly open, configure and activate the actual ALSA microphone. */
        snd_pcm_start(state->capture) < 0) /* Explicitly open, configure and activate the actual ALSA microphone. */
    { /* Release capture and loaded native inference state after startup failure. */
        fvm_linux_stt_close(state); /* Stop any partially opened microphone and free the model. */
        return FVM_ERROR; /* Preserve actual permissions, device-busy and format failures. */
    } /* Finish capture startup failure. */
    *out = state; /* Transfer actual capture/model ownership to the common STT session. */
    return FVM_OK; /* Poll now processes actual microphone speech; no hidden thread is created. */
} /* Finish real offline recognition startup. */
#endif /* Finish optional native implementation. */
