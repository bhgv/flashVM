/* OS: Android. Retrieve real recognition text as standard UTF-8 with paired final/partial state. */
#include "../android.hpp" /* Import Java/native transcript ownership. */
#ifdef __ANDROID__ /* Isolate actual JNI service calls. */
#include "flashvm/utf8.h" /* Validate actual provider output before publishing it. */
#include <new> /* Allocate retained text without exceptions or STL. */
#include <string.h> /* Copy a completed transcript into caller storage. */
/* Keep copied transcript ownership across size queries and insufficient buffers. */
FVM_Result fvm_android_stt_poll(void *opaque, char *text, size_t capacity, size_t *required, /* This method never opens or restarts the microphone. */
                                int *final) /* This method never opens or restarts the microphone. */
{ /* Begin nonblocking recognition polling. */
    FVM_AndroidSpeech *state = static_cast<FVM_AndroidSpeech *>(opaque); /* Recover the active recognizer helper. */
    if (!state->pending) /* Fetch Java text only when no previous native transcript is retained. */
    { /* Acquire one atomic text/final event from Java. */
        JNIEnv *env = 0; /* Receive a valid environment for this native thread. */
        bool attached = false; /* Track temporary thread attachment. */
        if (!fvm_android_speech_env(state->vm, &env, &attached)) /* Preserve unavailable VM failure. */
            return FVM_ERROR; /* Preserve unavailable VM failure. */
        jbyteArray bytes = static_cast<jbyteArray>(env->CallObjectMethod( /* Take an atomic final-marker plus UTF-8 transcript. */
            state->service, state->take)); /* Take an atomic final-marker plus UTF-8 transcript. */
        FVM_Result result = FVM_NOT_FOUND; /* An empty Java queue is normal. */
        if (bytes && !env->ExceptionCheck()) /* Copy an actual recognition event. */
        { /* Validate the helper's event layout before native allocation. */
            jsize size = env->GetArrayLength(bytes); /* Count marker and payload bytes from the actual Java array. */
            if (size > 0 && (unsigned long)size <= FVM_SPEECH_TEXT_LIMIT + 1) /* Bound native transcript storage. */
            { /* Retain a complete private native copy. */
                state->pending = /* Reserve payload plus NUL using the removed marker byte. */
                    new (std::nothrow) char[(size_t)size]; /* Reserve payload plus NUL using the removed marker byte. */
                if (state->pending) /* Continue only with actual allocated storage. */
                { /* Copy marker and standard UTF-8 payload separately. */
                    jbyte marker = 0; /* Receive final/partial state without signed-char ambiguity. */
                    env->GetByteArrayRegion(bytes, 0, 1, &marker); /* Read the event's atomic marker. */
                    env->GetByteArrayRegion(bytes, 1, size - 1, /* Copy text without JNI modified-UTF-8 conversion. */
                                            reinterpret_cast<jbyte *>( /* Copy text without JNI modified-UTF-8 conversion. */
                                                state->pending)); /* Copy text without JNI modified-UTF-8 conversion. */
                    state->pending[size - 1] = 0; /* Complete caller-compatible NUL termination. */
                    state->required = (size_t)size; /* Count payload bytes plus NUL. */
                    state->final = marker; /* Preserve the transcript's own final/partial state. */
                    result = (marker == 0 || marker == 1) && !memchr(state->pending, 0, (size_t)size - 1) && /* Reject malformed helper output. */
                                     fvm_utf8_validate(state->pending, (size_t)size - 1, 0) == FVM_UTF8_OK /* Reject malformed helper output. */
                                 ? FVM_OK /* Reject malformed helper output. */
                                 : FVM_ERROR; /* Reject malformed helper output. */
                } /* Finish native transcript copying. */
                else /* Preserve native allocation failure. */
                    result = FVM_OUT_OF_MEMORY; /* Preserve native allocation failure. */
            } /* Finish bounded event handling. */
            else /* Reject an invalid helper event size. */
                result = FVM_ERROR; /* Reject an invalid helper event size. */
        } /* Finish Java event retrieval. */
        else if (!env->ExceptionCheck()) /* Expose native recognition failure when no transcript is pending. */
            result = (FVM_Result)env->CallIntMethod( /* Expose native recognition failure when no transcript is pending. */
                state->service, state->status); /* Expose native recognition failure when no transcript is pending. */
        if (result == FVM_OK && !state->pending) /* A ready recognizer need not have text yet. */
            result = FVM_NOT_FOUND; /* A ready recognizer need not have text yet. */
        if (env->ExceptionCheck()) /* Convert JNI/provider exceptions to the public error boundary. */
        { /* Keep caller-owned JNI state free of pending exceptions. */
            env->ExceptionClear(); /* Report through C result codes instead. */
            result = FVM_ERROR; /* Preserve actual service/JNI failure. */
        } /* Finish exception conversion. */
        if (bytes) /* Release this call's temporary Java event reference. */
            env->DeleteLocalRef(bytes); /* Release this call's temporary Java event reference. */
        if (attached) /* Balance only this operation's native attachment. */
            state->vm->DetachCurrentThread(); /* Balance only this operation's native attachment. */
        if (result != FVM_OK) /* Reject partial native event state after any failure. */
        { /* Release an unsuccessful transcript copy. */
            delete[] state->pending; /* Avoid retaining invalid text across polls. */
            state->pending = 0; /* Restore the empty native pending state. */
            return result; /* Preserve empty-queue, allocation and service errors. */
        } /* Finish failed-event handling. */
    } /* Finish Java-to-native event transfer. */
    *required = state->required; /* Report the exact sufficient UTF-8 capacity. */
    *final = state->final; /* Return final/partial state paired with this transcript. */
    if (!capacity) /* Query mode retains this exact event. */
        return FVM_OK; /* Query mode retains this exact event. */
    if (capacity < state->required) /* An undersized buffer does not consume the native copy. */
        return FVM_INVALID_ARGUMENT; /* An undersized buffer does not consume the native copy. */
    memcpy(text, state->pending, /* Transfer complete standard UTF-8 plus NUL to host-owned storage. */
           state->required); /* Transfer complete standard UTF-8 plus NUL to host-owned storage. */
    delete[] state->pending; /* Release the consumed native transcript. */
    state->pending = 0; /* Permit retrieval of the next Java recognition event. */
    return FVM_OK; /* The host now owns an independent transcript copy. */
} /* Finish nonblocking transcript polling. */
#endif /* Finish Android-only implementation. */
