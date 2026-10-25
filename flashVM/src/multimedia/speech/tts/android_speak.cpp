/* OS: Android. Submit standard UTF-8 byte[] to the actual TextToSpeech helper. */
#include "../android.hpp" /* Import cached JNI methods and native ownership. */
#ifdef __ANDROID__        /* Isolate Android JVM calls. */
#include <string.h>       /* Measure already validated/bounded UTF-8. */


/* Avoid JNI modified-UTF-8 conversion so supplementary Unicode remains intact. */
FVM_Result fvm_android_tts_speak(void *opaque,                           /* Java marshals real synthesis onto its main Looper. */
                                 const char *text)                       /* Java marshals real synthesis onto its main Looper. */
{                                                                        /* Begin native text submission. */
    FVM_AndroidSpeech *state = static_cast<FVM_AndroidSpeech *>(opaque); /* Recover the real Java helper. */
    JNIEnv *env = 0;                                                     /* Receive this thread's JNI environment. */
    bool attached = false;                                               /* Track temporary native-thread attachment. */
    if (!fvm_android_speech_env(state->vm, &env, &attached))             /* Preserve unavailable VM failure. */
    {                                                                    /* Begin conditional branch: Preserve unavailable VM failure. */
        return FVM_ERROR;                                                /* Preserve unavailable VM failure. */
    } /* End conditional branch: Preserve unavailable VM failure. */
    FVM_Result result = (FVM_Result)env->CallIntMethod(                                /* Preserve initialization/service errors before enqueueing. */
                                                       state->service, state->status); /* Preserve initialization/service errors before enqueueing. */
    if (result == FVM_OK && !env->ExceptionCheck())                                    /* Submit only to a ready real synthesizer. */
    {                                                                                  /* Copy validated standard UTF-8 bytes into Java-owned memory. */
        jsize length = (jsize)strlen(text);                                            /* Common text limits keep this within jsize. */
        jbyteArray bytes = env->NewByteArray(length);                                  /* Allocate a native-compatible Java payload. */
        if (bytes)                                                                     /* Continue only after successful allocation. */
        {                                                                              /* Submit the actual text buffer. */
            env->SetByteArrayRegion(                                                   /* Copy without modified-UTF-8 transformations. */
                                    bytes, 0, length,                                  /* Copy without modified-UTF-8 transformations. */
                                    reinterpret_cast<const jbyte *>(text));            /* Copy without modified-UTF-8 transformations. */
            if (!env->ExceptionCheck())                                                /* Ask Java to queue real system speech. */
            {                                                                          /* Begin conditional branch: Ask Java to queue real system speech. */
                result = (FVM_Result)env->CallIntMethod(state->service, state->speak,  /* Ask Java to queue real system speech. */
                                                        bytes);                        /* Ask Java to queue real system speech. */
            } /* End conditional branch: Ask Java to queue real system speech. */
            env->DeleteLocalRef(bytes); /* Release this JNI call's temporary payload reference. */
        } /* Finish Java text submission. */
        else                            /* Preserve missing Java payload allocation. */
        {                               /* Begin conditional branch: Preserve missing Java payload allocation. */
            result = FVM_OUT_OF_MEMORY; /* Preserve missing Java payload allocation. */
        } /* End conditional branch: Preserve missing Java payload allocation. */
    } /* Finish ready-service submission. */
    if (env->ExceptionCheck()) /* Convert Java failures at the C ABI boundary. */
    {                          /* Do not leave an exception pending on a host-owned JNI thread. */
        env->ExceptionClear(); /* Preserve the host's ordinary error-return flow. */
        result = FVM_ERROR;    /* Report actual Java/provider failure. */
    } /* Finish Java exception handling. */
    if (attached)                         /* Balance only this operation's attachment. */
    {                                     /* Begin conditional branch: Balance only this operation's attachment. */
        state->vm->DetachCurrentThread(); /* Balance only this operation's attachment. */
    } /* End conditional branch: Balance only this operation's attachment. */
    return result; /* FVM_OK reports queued work, NOT_FOUND reports initialization still pending. */
} /* Finish native text submission. */
#endif /* Finish Android-only implementation. */
