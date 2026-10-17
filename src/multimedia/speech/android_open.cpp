/* OS: Android. Instantiate the supplied real Java speech-service helper. */
#include "android.hpp" /* Import JNI ownership and shared cleanup. */
#ifdef __ANDROID__ /* Isolate actual Android calls. */
#include <new> /* Allocate ownership without exceptions. */
#include <string.h> /* Clear partial JNI/session state. */
/* Create a helper on the calling native thread; Java marshals service work to its main Looper. */
FVM_Result fvm_android_speech_open(void *opaque, void **out, /* Host Context/class references remain host-owned. */
                                   bool tts) /* Host Context/class references remain host-owned. */
{ /* Begin explicit Java service creation. */
    const FVM_AndroidSpeechConfig *config = /* Recover persistent host configuration. */
        static_cast<const FVM_AndroidSpeechConfig *>(opaque); /* Recover persistent host configuration. */
    JavaVM *vm = static_cast<JavaVM *>(config->java_vm); /* Borrow the actual application VM. */
    JNIEnv *env = 0; /* Receive the current native thread's environment. */
    bool attached = false; /* Track temporary attachment ownership. */
    *out = 0; /* Publish no partially initialized native helper. */
    if (!fvm_android_speech_env(vm, &env, &attached)) /* Preserve unavailable JVM/thread failure. */
        return FVM_ERROR; /* Preserve unavailable JVM/thread failure. */
    FVM_AndroidSpeech *state = new (std::nothrow) FVM_AndroidSpeech; /* Allocate native helper ownership. */
    if (!state) /* Balance temporary attachment on allocation failure. */
    { /* Release only resources acquired by this operation. */
        if (attached) /* Preserve host-attached threads. */
            vm->DetachCurrentThread(); /* Preserve host-attached threads. */
        return FVM_OUT_OF_MEMORY; /* No service was created. */
    } /* Finish wrapper-allocation failure. */
    memset(state, 0, sizeof(*state)); /* Make native/global cleanup safe. */
    state->vm = vm; /* Borrow the VM for this session's lifetime. */
    jclass type = static_cast<jclass>(config->service_class); /* Borrow the host's global helper-class reference. */
    jmethodID constructor = env->GetMethodID( /* Resolve the supplied real helper constructor. */
        type, "<init>", "(Landroid/content/Context;)V"); /* Resolve the supplied real helper constructor. */
    if (constructor) /* Resolve cleanup before creating any native service. */
        state->close = env->GetMethodID(type, "close", "()V"); /* Resolve cleanup before creating any native service. */
    if (state->close) /* Resolve readiness and asynchronous errors. */
        state->status = env->GetMethodID(type, "status", "()I"); /* Resolve readiness and asynchronous errors. */
    if (state->status && tts) /* Resolve UTF-8 byte-array text submission. */
        state->speak = env->GetMethodID(type, "speak", "([B)I"); /* Resolve UTF-8 byte-array text submission. */
    if (state->speak && tts) /* Resolve real TTS cancellation. */
        state->stop = env->GetMethodID(type, "stop", "()I"); /* Resolve real TTS cancellation. */
    if (state->status && !tts) /* Resolve atomic text/final-result retrieval. */
        state->take = env->GetMethodID(type, "take", "()[B"); /* Resolve atomic text/final-result retrieval. */
    bool valid = /* Require the full documented Java helper ABI. */
        constructor && state->close && state->status && /* Require the full documented Java helper ABI. */
        (tts ? state->speak && state->stop : state->take != 0); /* Require the full documented Java helper ABI. */
    if (valid && !env->ExceptionCheck()) /* Create resources only after every required method resolves. */
    { /* Instantiate the actual service helper with the application Context. */
        jobject local = env->NewObject( /* Java checks permissions/provider availability before starting STT. */
            type, constructor, /* Java checks permissions/provider availability before starting STT. */
            static_cast<jobject>( /* Java checks permissions/provider availability before starting STT. */
                config->application_context)); /* Java checks permissions/provider availability before starting STT. */
        if (local) /* Promote successful construction to session-owned lifetime. */
        { /* Retain a global reference beyond this JNI call. */
            state->service = env->NewGlobalRef(local); /* Own the helper independently of native stack frames. */
            if (!state /* A failed global-reference allocation must still shut down the constructed Java service. */
                     ->service) /* A failed global-reference allocation must still shut down the constructed Java service. */
            { /* Clear allocation exceptions only long enough to schedule owned-resource cleanup. */
                if (env->ExceptionCheck()) /* JNI calls cannot run with a pending allocation exception. */
                    env->ExceptionClear(); /* JNI calls cannot run with a pending allocation exception. */
                env->CallVoidMethod(local, /* Close the real helper even though it could not be retained. */
                                    state->close); /* Close the real helper even though it could not be retained. */
            } /* Finish failed-global-reference cleanup. */
            env->DeleteLocalRef(local); /* Release the temporary local reference. */
        } /* Finish local-to-global promotion. */
    } /* Finish service construction. */
    bool failed = env->ExceptionCheck() || !state->service; /* Detect Java permission/service/ABI failures. */
    if (env->ExceptionCheck()) /* Convert Java failure to the documented C result boundary. */
        env->ExceptionClear(); /* Convert Java failure to the documented C result boundary. */
    if (attached) /* Balance only temporary native-thread attachment. */
        vm->DetachCurrentThread(); /* Balance only temporary native-thread attachment. */
    if (failed) /* Release every successfully acquired native/global resource. */
    { /* Do not publish unusable speech state. */
        fvm_android_speech_close(state); /* Schedule Java cleanup and release native ownership. */
        return FVM_ERROR; /* Caller can inspect app logs/service setup for the native cause. */
    } /* Finish failed service creation. */
    *out = state; /* Transfer the real asynchronous service helper to the session. */
    return FVM_OK; /* Readiness may still return NOT_FOUND until Java initialization completes. */
} /* Finish explicit Java service opening. */
#endif /* Finish Android-only implementation. */
