/* OS: Linux. Real optional ALSA microphone capture and static Vosk recognition. */
#ifndef FVM_STT_LINUX_VOSK_HPP /* Protect private recognizer declarations. */
#define FVM_STT_LINUX_VOSK_HPP /* Define private include protection. */
#include "../session.hpp"      /* Share the C89 provider ABI. */


/* Decode only the bounded single-string JSON result shape configured for Vosk. */
FVM_Result fvm_vosk_text(const char *, bool, char **); /* OS: portable parser; no microphone or model needed. */
#if defined(__linux__) && !defined(__ANDROID__) &&     /* Require actual static native SDK selection. */ \
    defined(FVM_HAS_LINUX_STT)                         /* Require actual static native SDK selection. */
extern "C"                                             /* Import real native libraries with C linkage. */
{                                                      /* Import real native libraries with C linkage. */
#include <alsa/asoundlib.h>                            /* Declare actual nonblocking PCM capture functions. */
#include <vosk_api.h>                                  /* Declare actual offline model/recognizer functions. */

} /* Finish native C linkage. */
#define FVM_VOSK_SAMPLE_RATE 16000           /* Feed explicit mono 16 kHz PCM to the installed model. */
#define FVM_VOSK_CAPTURE_FRAMES 2048         /* Bound audio processing work per poll call. */
#define FVM_VOSK_LATENCY_MICROSECONDS 500000 /* Request a half-second native ALSA capture buffer. */

struct FVM_LinuxStt                          /* Own one explicit microphone/model/recognizer lifecycle. */
{                                            /* Own one explicit microphone/model/recognizer lifecycle. */
    snd_pcm_t *capture;                      /* Own the real ALSA capture stream. */
    VoskModel *model;                        /* Own the loaded offline recognition model. */
    VoskRecognizer *recognizer;              /* Own this session's actual recognizer state. */
    short samples[FVM_VOSK_CAPTURE_FRAMES];  /* Keep one native-endian signed-16 capture chunk. */
    char *pending;                           /* Own one parsed recognized transcript until a sufficient host copy. */
    int final;                               /* Pair final/partial state with the retained transcript. */
}; /* Finish native capture/recognition ownership. */


/* Explicitly load a real model and open/configure the named microphone. */
FVM_Result fvm_linux_stt_open(void *, void **); /* OS: Linux ALSA/Vosk. */


/* Read a bounded PCM chunk and return actual recognized UTF-8 text. */
FVM_Result fvm_linux_stt_poll(void *, char *, size_t, size_t *, int *); /* OS: Linux ALSA/Vosk. */


/* Drop native microphone capture and release recognizer/model ownership. */
void fvm_linux_stt_close(void *); /* OS: Linux ALSA/Vosk. */
#endif                            /* Finish optional actual SDK declarations. */
#endif                            /* Finish private include protection. */
