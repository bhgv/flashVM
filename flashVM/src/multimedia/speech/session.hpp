/* OS: all supported targets. Unmanaged speech-session ownership and native callback declarations. */
#ifndef FVM_SPEECH_SESSION_HPP /* Protect the private speech interface. */
#define FVM_SPEECH_SESSION_HPP /* Define the private include guard. */
#include "flashvm/speech.h"    /* Share the C89 speech-provider ABI. */

struct FVM_TtsSession          /* Own one copied output table and native state. */
{                              /* Own one copied output table and native state. */
    FVM_TtsBackend backend;    /* Copy function pointers without taking ownership of host configuration. */
    void *native;              /* Own native provider resources until close. */
}; /* Finish output session state. */

struct FVM_SttSession       /* Own one copied recognition table and native state. */
{                           /* Own one copied recognition table and native state. */
    FVM_SttBackend backend; /* Copy recognizer callbacks. */
    void *native;           /* Own microphone/recognizer resources until close. */
}; /* Finish recognition session state. */


/* Fill a native Windows output table, or report unsupported on another target. */
FVM_Result fvm_tts_windows_backend(FVM_TtsBackend *); /* OS: Windows SAPI. */


/* Fill a native Windows recognition table, or report unsupported on another target. */
FVM_Result fvm_stt_windows_backend(FVM_SttBackend *); /* OS: Windows SAPI. */


/* Fill a native Apple output table, or report unsupported on another target. */
FVM_Result fvm_tts_macos_backend(FVM_TtsBackend *); /* OS: macOS Speech Synthesis framework. */
#endif                                              /* Finish the private guard. */
