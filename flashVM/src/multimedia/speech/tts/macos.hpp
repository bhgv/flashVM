/* OS: macOS. Private Core Foundation/Speech Synthesis ownership. */
#ifndef FVM_TTS_MACOS_HPP /* Protect native Apple speech declarations. */
#define FVM_TTS_MACOS_HPP /* Define the private include guard. */
#include "../session.hpp" /* Share the replaceable TTS ABI. */
#ifdef __APPLE__ /* Keep Apple framework types off other targets. */
#include <ApplicationServices/ApplicationServices.h> /* Declare actual Speech Synthesis and CFString APIs. */
struct FVM_MacosTts /* Own a real native voice channel. */
{ /* Own a real native voice channel. */
    SpeechChannel channel; /* Release with DisposeSpeechChannel after stopping speech. */
}; /* Finish native voice ownership. */
/* Create a channel for the configured system voice. */
FVM_Result fvm_macos_tts_open(void *, void **); /* OS: macOS Speech Synthesis. */
/* Convert strict UTF-8 to CFString and speak it through the native channel. */
FVM_Result fvm_macos_tts_speak(void *, const char *); /* OS: macOS Speech Synthesis. */
/* Stop this native channel's speech. */
FVM_Result fvm_macos_tts_stop(void *); /* OS: macOS Speech Synthesis. */
/* Stop and dispose the native channel. */
void fvm_macos_tts_close(void *); /* OS: macOS Speech Synthesis. */
#endif /* Finish Apple-only declarations. */
#endif /* Finish private header protection. */
