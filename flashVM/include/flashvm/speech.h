/* Replaceable speech ABI: explicit text playback and explicit microphone recognition. */
#ifndef FLASHVM_SPEECH_H                       /* Protect the public speech declarations. */
#define FLASHVM_SPEECH_H                       /* Define the public include guard. */
#include "flashvm.h"                           /* Reuse the C89 result and size types. */
#ifdef __cplusplus                             /* Preserve C linkage for C++ hosts. */
extern "C"                                     /* Begin the public speech ABI. */
{                                              /* Begin the public speech ABI. */
#endif                                         /* Finish linkage selection. */
#define FVM_SPEECH_ABI_VERSION 1UL             /* Exact TTS/STT callback-table version. */
#define FVM_SPEECH_TEXT_LIMIT 1048576UL        /* Maximum UTF-8 payload bytes accepted by the common TTS layer. */
typedef struct FVM_TtsSession FVM_TtsSession;  /* Own one native speech-output provider. */
typedef struct FVM_SttSession FVM_SttSession;  /* Own one explicitly opened recognition provider. */

typedef struct FVM_TtsBackend                  /* Copy these callbacks when opening a TTS session. */
{                                              /* Copy these callbacks when opening a TTS session. */
    unsigned long struct_size;                 /* Set to sizeof(FVM_TtsBackend). */
    unsigned long abi_version;                 /* Set to FVM_SPEECH_ABI_VERSION. */
    const char *os_name;                       /* Borrow a stable system/provider label. */
    void *userdata;                            /* Borrow native provider configuration for open. */
    FVM_Result (*open)(void *, void **);       /* Create an owned native output state; clean partial failures. */
    FVM_Result (*speak)(void *, const char *); /* Enqueue UTF-8 text through the real system/library. */
    FVM_Result (*stop)(void *);                /* Cancel this output's pending/current speech. */
    void (*close)(void *);                     /* Release one successfully opened native state. */
} FVM_TtsBackend;                              /* Finish the replaceable TTS contract. */

typedef struct FVM_SttBackend                  /* Copy these callbacks when explicitly opening microphone recognition. */
{                                              /* Copy these callbacks when explicitly opening microphone recognition. */
    unsigned long struct_size;                 /* Set to sizeof(FVM_SttBackend). */
    unsigned long abi_version;                 /* Set to FVM_SPEECH_ABI_VERSION. */
    const char *os_name;                       /* Borrow a stable system/provider label. */
    void *userdata;                            /* Borrow recognizer/device configuration for open. */
    FVM_Result (*open)(void *, void **);       /* Start recognition only when the host explicitly calls open. */
    FVM_Result (*poll)(void *, char *, size_t, /* Query/copy one UTF-8 transcript; retain it when the buffer is insufficient. */
                       size_t *,               /* Query/copy one UTF-8 transcript; retain it when the buffer is insufficient. */
                       int *);                 /* Query/copy one UTF-8 transcript; retain it when the buffer is insufficient. */
    void (*close)(void *);                     /* Stop recognition/capture and release native resources. */
} FVM_SttBackend;                              /* Finish the replaceable STT contract. */

#define FVM_SPEECH_UNIX_FILESYSTEM 1           /* Interpret endpoint bytes as a filesystem pathname without its terminator. */
#define FVM_SPEECH_UNIX_ABSTRACT 2             /* Insert the leading native NUL and preserve every supplied name byte. */

typedef struct FVM_SpeechUnixEndpoint          /* Keep socket namespace separate from binary name representation. */
{                                              /* Begin explicit Unix speech-service addressing. */
    unsigned long struct_size;                 /* Set to sizeof(FVM_SpeechUnixEndpoint). */
    int name_space;                            /* Select FILESYSTEM or ABSTRACT explicitly. */
    const void *name;                          /* Borrow raw name bytes; abstract bytes exclude the leading namespace NUL. */
    size_t length;                             /* Count exact name bytes, including embedded NULs in abstract names. */
} FVM_SpeechUnixEndpoint;                      /* Keep endpoint/name storage alive through native opening. */

/* Bind an explicit filesystem or abstract SSIP endpoint on Linux/Android without connecting yet. */
FVM_API FVM_Result                                   /* OS: Linux/Android Unix sockets; actual SSIP service required. */
fvm_tts_unix_backend(const FVM_SpeechUnixEndpoint *, /* OS: Linux/Android Unix sockets; actual SSIP service required. */
                     FVM_TtsBackend *);              /* OS: Linux/Android Unix sockets; actual SSIP service required. */

typedef struct FVM_LinuxSttConfig                    /* Explicit offline model and microphone configuration. */
{                                                    /* Explicit offline model and microphone configuration. */
    unsigned long struct_size;                       /* Set to sizeof(FVM_LinuxSttConfig). */
    const char *model_path;                          /* Borrow a real installed Vosk model directory; no download occurs. */
    const char *alsa_device;                         /* Borrow an explicit ALSA capture name, for example default or hw:1,0. */
} FVM_LinuxSttConfig;                                /* Keep configuration strings alive through native opening. */

/* Bind real ALSA capture plus a statically linked Vosk recognizer when its SDK is enabled. */
FVM_API FVM_Result                                /* OS: Linux, optional static SDK and an explicit model required. */
fvm_stt_linux_backend(                            /* OS: Linux, optional static SDK and an explicit model required. */
                      const FVM_LinuxSttConfig *, /* OS: Linux, optional static SDK and an explicit model required. */
                      FVM_SttBackend *);          /* OS: Linux, optional static SDK and an explicit model required. */

typedef struct FVM_AndroidSpeechConfig            /* Supply host-owned global JNI references; no Android types cross C89. */
{                                                 /* Supply host-owned global JNI references; no Android types cross C89. */
    unsigned long struct_size;                    /* Set to sizeof(FVM_AndroidSpeechConfig). */
    void *java_vm;                                /* Borrow the application's JavaVM pointer. */
    void *application_context;                    /* Borrow a global JNI reference to the application Context. */
    void *service_class;                          /* Borrow a global JNI reference to the supplied TTS or STT helper class. */
} FVM_AndroidSpeechConfig;                        /* Keep configuration/references alive through every native session. */

/* Bind the supplied real Android TextToSpeech helper without activating it yet. */
FVM_API FVM_Result                                       /* OS: Android, app Context/global helper class required. */
fvm_tts_android_backend(                                 /* OS: Android, app Context/global helper class required. */
                        const FVM_AndroidSpeechConfig *, /* OS: Android, app Context/global helper class required. */
                        FVM_TtsBackend *);               /* OS: Android, app Context/global helper class required. */
/* Bind the supplied real Android SpeechRecognizer helper without opening the microphone yet. */
FVM_API FVM_Result                                       /* OS: Android, host must grant RECORD_AUDIO first. */
fvm_stt_android_backend(const FVM_AndroidSpeechConfig *, /* OS: Android, host must grant RECORD_AUDIO first. */
                        FVM_SttBackend *);               /* OS: Android, host must grant RECORD_AUDIO first. */
/* Initialize the current target's native TTS provider, without speaking or opening a microphone. */
FVM_API FVM_Result                    /* OS: Windows SAPI, macOS Speech Synthesis; Linux uses explicit SSIP factory. */
fvm_tts_system_backend(               /* OS: Windows SAPI, macOS Speech Synthesis; Linux uses explicit SSIP factory. */
                       FVM_TtsBackend /* OS: Windows SAPI, macOS Speech Synthesis; Linux uses explicit SSIP factory. */
                           *);        /* OS: Windows SAPI, macOS Speech Synthesis; Linux uses explicit SSIP factory. */
/* Initialize the current target's native STT provider, without starting recognition. */
FVM_API FVM_Result                    /* OS: Windows SAPI or compiled macOS Speech; Linux/Android require explicit configuration. */
fvm_stt_system_backend(               /* OS: Windows SAPI or compiled macOS Speech; Linux/Android require explicit configuration. */
                       FVM_SttBackend /* OS: Windows SAPI or compiled macOS Speech; Linux/Android require explicit configuration. */
                           *);        /* OS: Windows SAPI or compiled macOS Speech; Linux/Android require explicit configuration. */
/* Initialize a Linux Speech Dispatcher client for an explicit Unix socket path; no daemon is spawned. */
FVM_API FVM_Result                       /* OS: Linux SSIP; path storage must outlive session opening. */
fvm_tts_linux_backend(                   /* OS: Linux SSIP; path storage must outlive session opening. */
                      const char *,      /* OS: Linux SSIP; path storage must outlive session opening. */
                      FVM_TtsBackend *); /* OS: Linux SSIP; path storage must outlive session opening. */


/* Open one native TTS provider and transfer session ownership to the caller. */
FVM_API FVM_Result fvm_tts_open(const FVM_TtsBackend *, FVM_TtsSession **); /* OS: selected provider. */


/* Enqueue one nonempty UTF-8 string; accepted work is not an audible-completion notification. */
FVM_API FVM_Result fvm_tts_speak(FVM_TtsSession *, const char *); /* OS: selected provider. */


/* Cancel only this provider session's speech queue. */
FVM_API FVM_Result fvm_tts_stop(FVM_TtsSession *); /* OS: selected provider. */


/* Stop/release one TTS session; null is harmless. */
FVM_API void fvm_tts_close(FVM_TtsSession *); /* OS: selected provider. */


/* Explicitly open recognition; the host must obtain microphone/speech permissions first. */
FVM_API FVM_Result fvm_stt_open(const FVM_SttBackend *, FVM_SttSession **); /* OS: selected provider. */


/* Poll one transcript: capacity includes NUL; zero capacity queries without consuming. */
FVM_API FVM_Result fvm_stt_poll(FVM_SttSession *, char *, size_t, /* OS: selected provider; required includes NUL, final is 0/1. */
                                size_t *,                         /* OS: selected provider; required includes NUL, final is 0/1. */
                                int *);                           /* OS: selected provider; required includes NUL, final is 0/1. */


/* Stop microphone recognition and release one session; null is harmless. */
FVM_API void fvm_stt_close(FVM_SttSession *); /* OS: selected provider. */
#ifdef __cplusplus                            /* Close C linkage for C++ hosts. */
} /* Finish the public speech ABI. */
#endif /* Finish linkage selection. */
#endif /* Finish the public include guard. */
