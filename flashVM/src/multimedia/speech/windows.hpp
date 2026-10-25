/* OS: Windows SAPI. Private native speech objects; all calls stay on their creating COM thread. */
#ifndef FVM_SPEECH_WINDOWS_HPP /* Protect the Windows speech declarations. */
#define FVM_SPEECH_WINDOWS_HPP /* Define the private header guard. */
#include "session.hpp"         /* Import public provider tables. */
#ifdef _WIN32                  /* Do not require a Windows SDK on other targets. */
#include <windows.h>           /* Declare base Windows types before dependent SAPI/MMSystem headers. */
#include <sapi.h>              /* Declare the real Windows voice and dictation interfaces. */

struct FVM_WindowsTts          /* Own one COM voice and its apartment reference. */
{                              /* Own one COM voice and its apartment reference. */
    ISpVoice *voice;           /* Own the native speech voice interface. */
    bool com;                  /* Balance only a successful CoInitializeEx call. */
}; /* Finish native voice ownership. */

struct FVM_WindowsStt          /* Own one shared recognizer context and dictation grammar. */
{                              /* Own one shared recognizer context and dictation grammar. */
    ISpRecognizer *recognizer; /* Own a reference to the configured system recognition service. */
    ISpRecoContext *context;   /* Own this session's event queue. */
    ISpRecoGrammar *grammar;   /* Own the active general-dictation grammar. */
    wchar_t *pending;          /* Own a CoTaskMem-allocated transcript until successful copying. */
    bool com;                  /* Balance this session's successful COM apartment initialization. */
}; /* Finish native recognizer ownership. */


/* Create the actual native voice on the calling COM thread. */
FVM_Result fvm_windows_tts_open(void *, void **); /* OS: Windows SAPI. */


/* Convert UTF-8 and enqueue native voice speech. */
FVM_Result fvm_windows_tts_speak(void *, const char *); /* OS: Windows SAPI. */


/* Purge native voice speech without closing the voice. */
FVM_Result fvm_windows_tts_stop(void *); /* OS: Windows SAPI. */


/* Purge, release the voice and balance COM initialization. */
void fvm_windows_tts_close(void *); /* OS: Windows SAPI. */


/* Create/activate real system dictation and its microphone input. */
FVM_Result fvm_windows_stt_open(void *, void **); /* OS: Windows SAPI. */


/* Receive/query/copy one real final dictation transcript. */
FVM_Result fvm_windows_stt_poll(void *, char *, size_t, size_t *, int *); /* OS: Windows SAPI. */


/* Deactivate dictation and release this session's COM references. */
void fvm_windows_stt_close(void *); /* OS: Windows SAPI. */
#endif                              /* Finish Windows-only declarations. */
#endif                              /* Finish the private include guard. */
