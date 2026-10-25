/* OS: Android. Balance optional native-thread JNI attachment for each operation. */
#include "android.hpp" /* Import the actual VM/environment APIs privately. */
#ifdef __ANDROID__     /* Isolate JNI calls from other toolchains. */


/* Return a valid environment without detaching a thread the host already attached. */
bool fvm_android_speech_env(JavaVM *vm, JNIEnv **env,                          /* The caller balances only newly attached threads. */
                            bool *attached)                                    /* The caller balances only newly attached threads. */
{                                                                              /* Begin environment acquisition. */
    *attached = false;                                                         /* Host-owned attachment is the default. */
    jint result = vm->GetEnv(reinterpret_cast<void **>(env), JNI_VERSION_1_6); /* Query the actual application VM. */
    if (result == JNI_OK)                                                      /* Preserve the host's existing attachment. */
    {                                                                          /* Begin conditional branch: Preserve the host's existing attachment. */
        return true;                                                           /* Preserve the host's existing attachment. */
    } /* End conditional branch: Preserve the host's existing attachment. */
    if (result != JNI_EDETACHED || vm->AttachCurrentThread(env, 0) != JNI_OK) /* Attach a native worker only when needed. */
    {                                                                         /* Begin conditional branch: Attach a native worker only when needed. */
        return false;                                                         /* Attach a native worker only when needed. */
    } /* End conditional branch: Attach a native worker only when needed. */
    *attached = true; /* Require this operation to detach its temporary attachment. */
    return true;      /* Return the current thread's valid JNI environment. */
} /* Finish environment acquisition. */
#endif /* Finish Android-only implementation. */
