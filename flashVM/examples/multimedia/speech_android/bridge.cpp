/* Android-only host integration tutorial: own Context/class globals and explicit native speech sessions. */
#ifdef __ANDROID__                  /* Compile this file in the Android application's JNI shared library only. */
#include "flashvm/speech.h"         /* Import C89-compatible providers and session lifetimes. */
#include <jni.h>                    /* Use real JavaVM, class-loader, byte-array and global-reference functions. */
#include <new>                      /* Allocate transcript copies without exceptions or STL. */
#include <string.h>                 /* Initialize persistent host configuration and inspect standard UTF-8 payloads. */

struct HostSpeech                   /* Own JNI configuration independently of the native service helper. */
{                                   /* Begin one host-side provider slot. */
    FVM_AndroidSpeechConfig config; /* Keep borrowed VM/Context/helper-class references alive through the session. */
    FVM_TtsSession *tts;            /* Own an optional explicit native TTS session. */
    FVM_SttSession *stt;            /* Own an optional explicit native recognition session. */
}; /* Finish host-side ownership. */

static HostSpeech output; /* This teaching host serializes all entrypoints on its Java/UI thread. */
static HostSpeech input;  /* Keep recognition state separate from text playback. */


/* Close sessions before releasing configuration globals; do not access either slot concurrently. */
static void release_slot(JNIEnv *env, HostSpeech *slot)                       /* The Java host owns VM lifetime through this cleanup. */
{                                                                             /* Begin deterministic host cleanup. */
    fvm_tts_close(slot->tts);                                                 /* Stop/release actual output before dropping its Context/class configuration. */
    fvm_stt_close(slot->stt);                                                 /* Stop/release actual microphone recognition before dropping JNI globals. */
    if (slot->config.application_context)                                     /* Release the host-owned application Context reference. */
    {                                                                         /* Begin conditional branch: Release the host-owned application Context reference. */
        env->DeleteGlobalRef(                                                 /* Release the host-owned application Context reference. */
                             static_cast<jobject>(                            /* Release the host-owned application Context reference. */
                                                  slot->config                /* Release the host-owned application Context reference. */
                                                      .application_context)); /* Release the host-owned application Context reference. */
    } /* End conditional branch: Release the host-owned application Context reference. */
    if (slot->config.service_class)                                            /* Release the host-owned helper-class reference. */
    {                                                                          /* Begin conditional branch: Release the host-owned helper-class reference. */
        env->DeleteGlobalRef(                                                  /* Release the host-owned helper-class reference. */
                             static_cast<jclass>(                              /* Release the host-owned helper-class reference. */
                                                 slot->config.service_class)); /* Release the host-owned helper-class reference. */
    } /* End conditional branch: Release the host-owned helper-class reference. */
    memset(slot, 0, sizeof(*slot)); /* Prevent stale session/global-reference reuse. */
} /* Finish host cleanup. */


/* Prepare real app/class references on a Java-originated call with the correct application class loader. */
static FVM_Result prepare_slot(JNIEnv *env, jobject context, /* The caller serializes this teaching host on the UI thread. */
                               const char *helper,           /* The caller serializes this teaching host on the UI thread. */
                               HostSpeech *slot)             /* The caller serializes this teaching host on the UI thread. */
{                                                            /* Begin explicit native preparation. */
    JavaVM *vm = 0;                                          /* Borrow the actual application VM. */
    release_slot(env, slot);                                 /* Close a previous session before replacing its configuration. */
    if (!context || env->GetJavaVM(&vm) != JNI_OK)           /* Require actual live app state. */
    {                                                        /* Begin conditional branch: Require actual live app state. */
        return FVM_INVALID_ARGUMENT;                         /* Require actual live app state. */
    } /* End conditional branch: Require actual live app state. */
    jclass local =                 /* Resolve the packaged supplied helper through the Java caller's class loader. */
        env->FindClass(helper);    /* Resolve the packaged supplied helper through the Java caller's class loader. */
    if (!local)                    /* Preserve missing Java source/packaging failure. */
    {                              /* Convert JNI exception into an ordinary native result for this tutorial. */
        if (env->ExceptionCheck()) /* Do not leave class lookup exceptions pending in the host. */
        {                          /* Begin conditional branch: Do not leave class lookup exceptions pending in the host. */
            env->ExceptionClear(); /* Do not leave class lookup exceptions pending in the host. */
        } /* End conditional branch: Do not leave class lookup exceptions pending in the host. */
        return FVM_ERROR; /* README documents copying helper sources into the app. */
    } /* Finish class lookup failure handling. */
    slot->config.struct_size = sizeof(slot->config);               /* Identify the exact persistent JNI configuration. */
    slot->config.java_vm = vm;                                     /* Borrow the VM until both sessions are closed. */
    slot->config.application_context = env->NewGlobalRef(context); /* Own a real application Context reference. */
    if (!env->ExceptionCheck())                                    /* Retain the actual helper class beyond this JNI stack frame. */
    {                                                              /* Begin conditional branch: Retain the actual helper class beyond this JNI stack frame. */
        slot->config.service_class =                               /* Retain the actual helper class beyond this JNI stack frame. */
            env->NewGlobalRef(local);                              /* Retain the actual helper class beyond this JNI stack frame. */
    } /* End conditional branch: Retain the actual helper class beyond this JNI stack frame. */
    env->DeleteLocalRef(local);              /* Release temporary class lookup ownership. */
    if (env->ExceptionCheck() ||             /* Preserve global-reference allocation failure. */
        !slot->config.application_context || /* Preserve global-reference allocation failure. */
        !slot->config.service_class)         /* Preserve global-reference allocation failure. */
    {                                        /* Release successful siblings after clearing the Java allocation exception. */
        if (env->ExceptionCheck())           /* Permit deterministic JNI cleanup calls. */
        {                                    /* Begin conditional branch: Permit deterministic JNI cleanup calls. */
            env->ExceptionClear();           /* Permit deterministic JNI cleanup calls. */
        } /* End conditional branch: Permit deterministic JNI cleanup calls. */
        release_slot(env, slot);  /* Release any retained global references. */
        return FVM_OUT_OF_MEMORY; /* Do not open native speech with partial configuration. */
    } /* Finish preparation-allocation failure handling. */
    return FVM_OK; /* Persistent host references are ready for an explicit provider open. */
} /* Finish native preparation. */

/* Explicitly create the real Android TTS service; speak may return NOT_FOUND until initialization completes. */
extern "C" JNIEXPORT jint JNICALL                                                       /* Java passes getApplicationContext(), never an Activity. */
Java_org_flashvm_example_SpeechHost_openTts(                                            /* Java passes getApplicationContext(), never an Activity. */
                                            JNIEnv *env, jclass type,                   /* Java passes getApplicationContext(), never an Activity. */
                                            jobject                                     /* Java passes getApplicationContext(), never an Activity. */
                                                context)                                /* Java passes getApplicationContext(), never an Activity. */
{                                                                                       /* Begin explicit TTS setup. */
    (void)type;                                                                         /* This static teaching entrypoint has no Java instance state. */
    FVM_TtsBackend backend;                                                             /* Build a native provider table from actual app references. */
    FVM_Result result = prepare_slot(                                                   /* Resolve the supplied real TextToSpeech helper. */
                                     env, context, "org/flashvm/multimedia/FlashVMTts", /* Resolve the supplied real TextToSpeech helper. */
                                     &output);                                          /* Resolve the supplied real TextToSpeech helper. */
    if (result == FVM_OK)                                                               /* Bind actual JNI/system-service functions. */
    {                                                                                   /* Begin conditional branch: Bind actual JNI/system-service functions. */
        result = fvm_tts_android_backend(&output.config, &backend);                     /* Bind actual JNI/system-service functions. */
    } /* End conditional branch: Bind actual JNI/system-service functions. */
    if (result == FVM_OK)                             /* Explicitly create the real Android synthesizer. */
    {                                                 /* Begin conditional branch: Explicitly create the real Android synthesizer. */
        result = fvm_tts_open(&backend, &output.tts); /* Explicitly create the real Android synthesizer. */
    } /* End conditional branch: Explicitly create the real Android synthesizer. */
    if (result != FVM_OK)           /* Release configuration on unsuccessful opening. */
    {                               /* Begin conditional branch: Release configuration on unsuccessful opening. */
        release_slot(env, &output); /* Release configuration on unsuccessful opening. */
    } /* End conditional branch: Release configuration on unsuccessful opening. */
    return (jint)result; /* Java reports setup/readiness failures normally. */
} /* Finish explicit TTS setup. */

/* Copy standard UTF-8 from Java, then enqueue actual system synthesis. */
extern "C" JNIEXPORT jint JNICALL                                   /* This avoids GetStringUTFChars modified-UTF-8 semantics. */
Java_org_flashvm_example_SpeechHost_speak(                          /* This avoids GetStringUTFChars modified-UTF-8 semantics. */
                                          JNIEnv *env, jclass type, /* This avoids GetStringUTFChars modified-UTF-8 semantics. */
                                          jbyteArray                /* This avoids GetStringUTFChars modified-UTF-8 semantics. */
                                              bytes)                /* This avoids GetStringUTFChars modified-UTF-8 semantics. */
{                                                                   /* Begin actual speech submission. */
    (void)type;                                                     /* No Java instance ownership is needed. */
    if (!bytes)                                                     /* Require an actual UTF-8 byte array. */
    {                                                               /* Begin conditional branch: Require an actual UTF-8 byte array. */
        return FVM_INVALID_ARGUMENT;                                /* Require an actual UTF-8 byte array. */
    } /* End conditional branch: Require an actual UTF-8 byte array. */
    jsize length = env->GetArrayLength(bytes);                        /* Count actual standard UTF-8 payload bytes. */
    if (length <= 0 || (unsigned long)length > FVM_SPEECH_TEXT_LIMIT) /* Bound host-side allocation. */
    {                                                                 /* Begin conditional branch: Bound host-side allocation. */
        return FVM_INVALID_ARGUMENT;                                  /* Bound host-side allocation. */
    } /* End conditional branch: Bound host-side allocation. */
    char *text = new (std::nothrow) char[(size_t)length + 1]; /* Own a terminated native text copy for this call. */
    if (!text)                                                /* Preserve ordinary allocation failure. */
    {                                                         /* Begin conditional branch: Preserve ordinary allocation failure. */
        return FVM_OUT_OF_MEMORY;                             /* Preserve ordinary allocation failure. */
    } /* End conditional branch: Preserve ordinary allocation failure. */
    env->GetByteArrayRegion(bytes, 0, length, reinterpret_cast<jbyte *>(text)); /* Copy standard UTF-8 unchanged. */
    text[length] = 0;                                                           /* Add the C ABI string terminator. */
    FVM_Result result = FVM_ERROR;                                              /* Preserve a JNI copy failure without invoking the system voice. */
    if (!env->ExceptionCheck())                                                 /* Reject embedded NUL and submit actual validated text. */
    {                                                                           /* Begin conditional branch: Reject embedded NUL and submit actual validated text. */
        result = memchr(text, 0, (size_t)length)                                /* Reject embedded NUL and submit actual validated text. */
                     ? FVM_INVALID_ARGUMENT                                     /* Reject embedded NUL and submit actual validated text. */
                     : fvm_tts_speak(output.tts, text);                         /* Reject embedded NUL and submit actual validated text. */
    } /* End conditional branch: Reject embedded NUL and submit actual validated text. */
    if (env->ExceptionCheck()) /* Convert Java failures into ordinary result codes. */
    {                          /* Begin conditional branch: Convert Java failures into ordinary result codes. */
        env->ExceptionClear(); /* Convert Java failures into ordinary result codes. */
    } /* End conditional branch: Convert Java failures into ordinary result codes. */
    delete[] text;       /* Native service helpers copied input before returning. */
    return (jint)result; /* NOT_FOUND means initialization is pending; do not silently discard requested text. */
} /* Finish actual speech submission. */

/* Explicitly activate actual microphone recognition after the Java host grants RECORD_AUDIO. */
extern "C" JNIEXPORT jint JNICALL                                                       /* No implicit permission dialog occurs in native code. */
Java_org_flashvm_example_SpeechHost_openStt(                                            /* No implicit permission dialog occurs in native code. */
                                            JNIEnv *env, jclass type,                   /* No implicit permission dialog occurs in native code. */
                                            jobject context)                            /* No implicit permission dialog occurs in native code. */
{                                                                                       /* Begin explicit STT setup. */
    (void)type;                                                                         /* This teaching host uses one serialized native recognition slot. */
    FVM_SttBackend backend;                                                             /* Receive real Android recognition callbacks. */
    FVM_Result result = prepare_slot(                                                   /* Resolve the supplied real SpeechRecognizer helper. */
                                     env, context, "org/flashvm/multimedia/FlashVMStt", /* Resolve the supplied real SpeechRecognizer helper. */
                                     &input);                                           /* Resolve the supplied real SpeechRecognizer helper. */
    if (result == FVM_OK)                                                               /* Bind actual app/service JNI references. */
    {                                                                                   /* Begin conditional branch: Bind actual app/service JNI references. */
        result = fvm_stt_android_backend(&input.config, &backend);                      /* Bind actual app/service JNI references. */
    } /* End conditional branch: Bind actual app/service JNI references. */
    if (result == FVM_OK)                            /* Explicitly schedule real microphone recognition. */
    {                                                /* Begin conditional branch: Explicitly schedule real microphone recognition. */
        result = fvm_stt_open(&backend, &input.stt); /* Explicitly schedule real microphone recognition. */
    } /* End conditional branch: Explicitly schedule real microphone recognition. */
    if (result != FVM_OK)          /* Release retained configuration after failed opening. */
    {                              /* Begin conditional branch: Release retained configuration after failed opening. */
        release_slot(env, &input); /* Release retained configuration after failed opening. */
    } /* End conditional branch: Release retained configuration after failed opening. */
    return (jint)result; /* The Java host reports actual permission/provider failure. */
} /* Finish explicit STT setup. */

/* Retrieve actual UTF-8 with a paired final marker; null means no text or a reported native error. */
extern "C" JNIEXPORT jbyteArray JNICALL                   /* A production UI should expose a separate status/error channel too. */
Java_org_flashvm_example_SpeechHost_poll(                 /* A production UI should expose a separate status/error channel too. */
                                         JNIEnv *env,     /* A production UI should expose a separate status/error channel too. */
                                         jclass           /* A production UI should expose a separate status/error channel too. */
                                             type)        /* A production UI should expose a separate status/error channel too. */
{                                                         /* Begin explicit transcript polling. */
    (void)type;                                           /* No Java object lifetime is retained here. */
    size_t required = 0;                                  /* Query exact native transcript bytes including NUL. */
    int final = 0;                                        /* Receive actual final/partial state. */
    FVM_Result result =                                   /* Preserve a stable pending transcript across calls. */
        fvm_stt_poll(input.stt, 0, 0, &required, &final); /* Preserve a stable pending transcript across calls. */
    if (result == FVM_NOT_FOUND)                          /* Empty native recognition queue is normal. */
    {                                                     /* Begin conditional branch: Empty native recognition queue is normal. */
        return 0;                                         /* Empty native recognition queue is normal. */
    } /* End conditional branch: Empty native recognition queue is normal. */
    if (result != FVM_OK)                                                                      /* Report actual recognition failure through the Java host's error channel. */
    {                                                                                          /* Keep errors distinguishable from ordinary no-text status. */
        jclass error =                                                                         /* Use an ordinary host exception for this tutorial boundary. */
            env->FindClass(                                                                    /* Use an ordinary host exception for this tutorial boundary. */
                           "java/lang/IllegalStateException");                                 /* Use an ordinary host exception for this tutorial boundary. */
        if (error)                                                                             /* Preserve actual native failure rather than fabricating text. */
        {                                                                                      /* Begin conditional branch: Preserve actual native failure rather than fabricating text. */
            env->ThrowNew(                                                                     /* Preserve actual native failure rather than fabricating text. */
                          error,                                                               /* Preserve actual native failure rather than fabricating text. */
                          "Native recognition poll failed; check permission/provider status"); /* Preserve actual native failure rather than fabricating text. */
        } /* End conditional branch: Preserve actual native failure rather than fabricating text. */
        if (error)                      /* Release temporary exception-class ownership. */
        {                               /* Begin conditional branch: Release temporary exception-class ownership. */
            env->DeleteLocalRef(error); /* Release temporary exception-class ownership. */
        } /* End conditional branch: Release temporary exception-class ownership. */
        return 0; /* Java receives the pending exception. */
    } /* Finish recognition error reporting. */
    char *text = new (std::nothrow) char[required];                     /* Own a complete stable native transcript copy. */
    if (!text)                                                          /* Preserve host allocation failure as an explicit Java error. */
    {                                                                   /* Do not confuse failure with an ordinary empty recognition queue. */
        jclass error = env->FindClass("java/lang/OutOfMemoryError");    /* Use the host's actual allocation error type. */
        if (error)                                                      /* Preserve the failed actual transcript operation. */
        {                                                               /* Begin conditional branch: Preserve the failed actual transcript operation. */
            env->ThrowNew(                                              /* Preserve the failed actual transcript operation. */
                          error,                                        /* Preserve the failed actual transcript operation. */
                          "Unable to allocate native transcript copy"); /* Preserve the failed actual transcript operation. */
        } /* End conditional branch: Preserve the failed actual transcript operation. */
        if (error)                      /* Release temporary exception-class ownership. */
        {                               /* Begin conditional branch: Release temporary exception-class ownership. */
            env->DeleteLocalRef(error); /* Release temporary exception-class ownership. */
        } /* End conditional branch: Release temporary exception-class ownership. */
        return 0; /* Java receives the pending allocation exception. */
    } /* Finish allocation-failure reporting. */
    result = fvm_stt_poll(input.stt, text, required, &required, &final);  /* Copy the same retained actual transcript. */
    jbyteArray bytes = 0;                                                 /* Publish only a complete Java event. */
    if (result == FVM_OK)                                                 /* Return marker plus actual standard UTF-8 without a C terminator. */
    {                                                                     /* Build Java-owned event bytes. */
        bytes = env->NewByteArray((jsize)required);                       /* One marker replaces the native trailing NUL in the byte count. */
        if (bytes)                                                        /* Copy only to a successfully allocated Java array. */
        {                                                                 /* Transfer finality and transcript in one event. */
            jbyte marker = (jbyte) final;                                 /* Preserve actual final/partial state. */
            env->SetByteArrayRegion(bytes, 0, 1, &marker);                /* Store the event marker first. */
            if (!env->ExceptionCheck())                                   /* Copy actual recognized UTF-8 bytes. */
            {                                                             /* Begin conditional branch: Copy actual recognized UTF-8 bytes. */
                env->SetByteArrayRegion(bytes, 1, (jsize)required - 1,    /* Copy actual recognized UTF-8 bytes. */
                                        reinterpret_cast<jbyte *>(text)); /* Copy actual recognized UTF-8 bytes. */
            } /* End conditional branch: Copy actual recognized UTF-8 bytes. */
        } /* Finish Java event copying. */
    } /* Finish successful transcript transfer. */
    if (result != FVM_OK)                                                 /* Do not confuse an actual second-copy error with an empty transcript queue. */
    {                                                                     /* Report native copy failure through the host exception boundary. */
        jclass error = env->FindClass("java/lang/IllegalStateException"); /* Use an ordinary Java host error type. */
        if (error)                                                        /* Preserve actual query/copy failure. */
        {                                                                 /* Begin conditional branch: Preserve actual query/copy failure. */
            env->ThrowNew(error, "Native transcript copy failed");        /* Preserve actual query/copy failure. */
        } /* End conditional branch: Preserve actual query/copy failure. */
        if (error)                      /* Release temporary exception-class ownership. */
        {                               /* Begin conditional branch: Release temporary exception-class ownership. */
            env->DeleteLocalRef(error); /* Release temporary exception-class ownership. */
        } /* End conditional branch: Release temporary exception-class ownership. */
    } /* Finish copy-error reporting. */
    delete[] text; /* Release the temporary native copy after Java owns its bytes. */
    return bytes;  /* Java decodes marker/payload using StandardCharsets.UTF_8. */
} /* Finish actual transcript polling. */

/* Explicit Activity/app lifecycle cleanup releases microphone/output before JNI globals. */
extern "C" JNIEXPORT void JNICALL                      /* Call before the host destroys its VM/application-owned bridge lifetime. */
Java_org_flashvm_example_SpeechHost_close(             /* Call before the host destroys its VM/application-owned bridge lifetime. */
                                          JNIEnv *env, /* Call before the host destroys its VM/application-owned bridge lifetime. */
                                          jclass       /* Call before the host destroys its VM/application-owned bridge lifetime. */
                                              type)    /* Call before the host destroys its VM/application-owned bridge lifetime. */
{                                                      /* Begin deterministic host lifecycle cleanup. */
    (void)type;                                        /* Cleanup is static and serialized on the host UI thread. */
    release_slot(env, &input);                         /* Stop actual recognition/capture first. */
    release_slot(env, &output);                        /* Stop actual speech output and release remaining JNI ownership. */
} /* Finish host lifecycle cleanup. */
#endif /* Finish Android-only integration source. */
