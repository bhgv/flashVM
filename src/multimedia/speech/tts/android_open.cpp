/* OS: Android. Explicitly open the real TextToSpeech helper. */
#include "../android.hpp" /* Import shared JNI service construction. */
#ifdef __ANDROID__ /* Compile only with the actual Android SDK. */
/* Select the corresponding Java helper contract without changing native ownership. */
FVM_Result fvm_android_tts_open(void *config, void **out) /* Only STT opening can activate microphone recognition. */
{ /* Begin explicit provider opening. */
    return fvm_android_speech_open(config, out, true); /* Construct the real selected Java service. */
} /* Finish explicit provider opening. */
#endif /* Finish Android-only implementation. */
