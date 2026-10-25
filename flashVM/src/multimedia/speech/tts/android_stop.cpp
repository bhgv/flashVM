/* OS: Android. Schedule real TextToSpeech cancellation through the supplied helper. */
#include "../android.hpp" /* Import cached JNI ownership. */
#ifdef __ANDROID__        /* Isolate actual Java service calls. */


/* Return scheduling/provider status rather than fabricating audible completion. */
FVM_Result fvm_android_tts_stop(void *opaque)                            /* Java runs native-service work on the main Looper. */
{                                                                        /* Begin cancellation submission. */
    FVM_AndroidSpeech *state = static_cast<FVM_AndroidSpeech *>(opaque); /* Recover the actual Java service. */
    JNIEnv *env = 0;                                                     /* Acquire the current thread's JNI environment. */
    bool attached = false;                                               /* Track temporary attachment ownership. */
    if (!fvm_android_speech_env(state->vm, &env, &attached))             /* Preserve VM/thread failure. */
    {                                                                    /* Begin conditional branch: Preserve VM/thread failure. */
        return FVM_ERROR;                                                /* Preserve VM/thread failure. */
    } /* End conditional branch: Preserve VM/thread failure. */
    FVM_Result result =                                              /* Schedule real platform speech cancellation. */
        (FVM_Result)env->CallIntMethod(state->service, state->stop); /* Schedule real platform speech cancellation. */
    if (env->ExceptionCheck())                                       /* Convert native-service exceptions to public errors. */
    {                                                                /* Preserve ordinary C ABI control flow. */
        env->ExceptionClear();                                       /* Do not leave a Java exception pending for the caller. */
        result = FVM_ERROR;                                          /* Report the failed system bridge call. */
    } /* Finish Java exception handling. */
    if (attached)                         /* Preserve any original host attachment. */
    {                                     /* Begin conditional branch: Preserve any original host attachment. */
        state->vm->DetachCurrentThread(); /* Preserve any original host attachment. */
    } /* End conditional branch: Preserve any original host attachment. */
    return result; /* Return the helper's accepted-work or failure status. */
} /* Finish cancellation submission. */
#endif /* Finish Android-only implementation. */
