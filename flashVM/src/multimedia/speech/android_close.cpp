/* OS: Android. Stop real Java speech/capture services and release JNI ownership. */
#include "android.hpp" /* Import native helper/global-reference ownership. */
#ifdef __ANDROID__     /* Isolate actual JVM cleanup. */


/* Java close schedules shutdown on its main Looper; queued Runnables retain their Java receiver. */
void fvm_android_speech_close(void *opaque)                              /* Host must keep the VM alive through cleanup. */
{                                                                        /* Begin native helper cleanup. */
    FVM_AndroidSpeech *state = static_cast<FVM_AndroidSpeech *>(opaque); /* Recover owned JNI state. */
    JNIEnv *env = 0;                                                     /* Acquire the current native thread's environment. */
    bool attached = false;                                               /* Track optional worker attachment. */
    if (fvm_android_speech_env(state->vm, &env, &attached))              /* Release actual Java ownership while the VM is alive. */
    {                                                                    /* Schedule real service shutdown before dropping the global reference. */
        if (state->service)                                              /* Skip absent resources after failed construction. */
        {                                                                /* Close the successfully constructed helper. */
            env->CallVoidMethod(state->service, state->close);           /* Java stops output or cancels/destroys recognition. */
            if (env->ExceptionCheck())                                   /* Void cleanup cannot propagate a Java exception across C ABI. */
            {                                                            /* Begin conditional branch: Void cleanup cannot propagate a Java exception across C ABI. */
                env->ExceptionClear();                                   /* Void cleanup cannot propagate a Java exception across C ABI. */
            } /* End conditional branch: Void cleanup cannot propagate a Java exception across C ABI. */
            env->DeleteGlobalRef(state->service); /* Release this session's owned Java reference. */
        } /* Finish Java helper release. */
        if (attached)                         /* Balance only temporary native-thread attachment. */
        {                                     /* Begin conditional branch: Balance only temporary native-thread attachment. */
            state->vm->DetachCurrentThread(); /* Balance only temporary native-thread attachment. */
        } /* End conditional branch: Balance only temporary native-thread attachment. */
    } /* Finish JVM cleanup; a dead VM violates the host lifetime contract. */
    delete[] state->pending; /* Release any transcript retained after a size query. */
    delete state;            /* Release unmanaged native ownership. */
} /* Finish native helper cleanup. */
#endif /* Finish Android-only implementation. */
