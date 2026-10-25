/* OS: Android NDK. Private JNI ownership for the supplied real Java speech helpers. */
#ifndef FVM_SPEECH_ANDROID_HPP /* Protect native JNI declarations. */
#define FVM_SPEECH_ANDROID_HPP /* Define private include protection. */
#include "session.hpp"         /* Share native speech-provider tables. */
#ifdef __ANDROID__             /* Keep Android SDK types private to Android builds. */
#include <jni.h>               /* Use actual VM/environment/global-reference APIs. */

struct FVM_AndroidSpeech       /* Own one Java helper and optional pending transcript. */
{                              /* Own one Java helper and optional pending transcript. */
    JavaVM *vm;                /* Borrow the live application VM. */
    jobject service;           /* Own a global JNI reference to the native-service helper. */
    jmethodID status;          /* Cache the helper's readiness/error method. */
    jmethodID speak;           /* Cache TTS enqueue method; null for STT. */
    jmethodID stop;            /* Cache TTS cancellation method; null for STT. */
    jmethodID take;            /* Cache STT transcript retrieval method; null for TTS. */
    jmethodID close;           /* Cache native-service cleanup scheduling. */
    char *pending;             /* Own copied UTF-8 transcript bytes until a successful host copy. */
    size_t required;           /* Count pending transcript bytes including NUL. */
    int final;                 /* Preserve final/partial state paired atomically with pending text. */
}; /* Finish JNI session ownership. */


/* Acquire the current thread's environment, attaching only when necessary. */
bool fvm_android_speech_env(JavaVM *, JNIEnv **, bool *); /* OS: Android JNI. */


/* Create a real supplied Java service helper; recording begins only for STT open. */
FVM_Result fvm_android_speech_open(void *, void **, bool); /* OS: Android JNI. */


/* Open the real TextToSpeech helper. */
FVM_Result fvm_android_tts_open(void *, void **); /* OS: Android JNI. */


/* Submit UTF-8 as byte[] without JNI modified-UTF-8 corruption. */
FVM_Result fvm_android_tts_speak(void *, const char *); /* OS: Android TextToSpeech. */


/* Schedule actual TextToSpeech cancellation. */
FVM_Result fvm_android_tts_stop(void *); /* OS: Android TextToSpeech. */


/* Open the real SpeechRecognizer helper explicitly. */
FVM_Result fvm_android_stt_open(void *, void **); /* OS: Android SpeechRecognizer. */


/* Retain/copy a real Java transcript returned as UTF-8 byte[]. */
FVM_Result fvm_android_stt_poll(void *, char *, size_t, size_t *, int *); /* OS: Android SpeechRecognizer. */


/* Schedule service shutdown and release native/global ownership. */
void fvm_android_speech_close(void *); /* OS: Android JNI. */
#endif                                 /* Finish Android-only declarations. */
#endif                                 /* Finish private include protection. */
