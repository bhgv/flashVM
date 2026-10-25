/* macOS 10.15+ host integration: explicitly request permissions, then open real recognition on the main thread. */
#include "flashvm/speech.h"           /* Import the C89-compatible native speech API. */
#import <AVFoundation/AVFoundation.h> /* Request actual microphone capture authorization. */
#import <Foundation/Foundation.h>     /* Use the real application main-thread and native object environment. */
#import <Speech/Speech.h>             /* Request actual system speech-recognition authorization. */
#import <dispatch/dispatch.h>         /* Return preparation/open completion to the application's main queue. */


/* Transfer a successfully opened session to the host callback; userdata remains host-owned until completion. */
typedef void (*SpeechPrepared)(FVM_Result, FVM_SttSession *, /* The callback must eventually close a successful native session. */
                               void *);                      /* The callback must eventually close a successful native session. */

/* Call from an explicit user action in a properly configured/signed macOS application. */
FVM_Result                                      /* This function intentionally demonstrates real system permission prompts. */
example_macos_prepare_stt(SpeechPrepared ready, /* This function intentionally demonstrates real system permission prompts. */
                          void *userdata)       /* This function intentionally demonstrates real system permission prompts. */
{                                               /* Begin explicit host preparation, separate from the library's nonprompting open operation. */
    if (!ready || ![NSThread isMainThread])     /* Require a real main-thread app callback owner. */
    {                                           /* Begin conditional branch: Require a real main-thread app callback owner. */
        return FVM_INVALID_ARGUMENT;            /* Require a real main-thread app callback owner. */
    } /* End conditional branch: Require a real main-thread app callback owner. */
    [SFSpeechRecognizer                                                                                                            /* Ask the actual system for speech-recognition authorization. */
        requestAuthorization:^(                                                                                                    /* Ask the actual system for speech-recognition authorization. */
                               SFSpeechRecognizerAuthorizationStatus                                                               /* Ask the actual system for speech-recognition authorization. */
                                   speech) {                                                                                       /* Ask the actual system for speech-recognition authorization. */
                                             if (speech != SFSpeechRecognizerAuthorizationStatusAuthorized)                        /* Respect actual denial/restriction rather than retrying silently. */
                                             {                                                                                     /* Respect actual denial/restriction rather than retrying silently. */
                                                 dispatch_async(dispatch_get_main_queue(), ^{                                      /* Report denied preparation on the host main thread. */
                                                                                              ready(FVM_UNSUPPORTED, 0, userdata); /* Report denied preparation on the host main thread. */
                                                 });                                                                               /* Report denied preparation on the host main thread. */
                                                 return;                                                                           /* Never request/open microphone capture after denied speech authorization. */
                                             } /* Finish speech authorization failure handling. */
                                             [AVCaptureDevice                                                                                          /* Continue the same native operation with the listed typed arguments. */
                                                 requestAccessForMediaType:AVMediaTypeAudio                                                            /* Ask the real system for microphone access explicitly. */
                                                         completionHandler:^(                                                                          /* Ask the real system for microphone access explicitly. */
                                                                             BOOL                                                                      /* Ask the real system for microphone access explicitly. */
                                                                                 granted) {                                                            /* Ask the real system for microphone access explicitly. */
                                                                                            dispatch_async(                                            /* Serialize actual recognition lifecycle with the host main loop. */
                                                                                                           dispatch_get_main_queue(),                  /* Serialize actual recognition lifecycle with the host main loop. */
                                                                                                           ^{                                          /* Serialize actual recognition lifecycle with the host main loop. */
                                                                                                              if (!granted)                            /* Continue the same native operation with the listed typed arguments. */
                                                                                                              {                                        /* Do not open capture without a real microphone grant. */
                                                                                                                  ready(FVM_UNSUPPORTED, 0, userdata); /* Do not open capture without a real microphone grant. */
                                                                                                                  return;                              /* Do not open capture without a real microphone grant. */
                                                                                                              } /* Do not open capture without a real microphone grant. */
                                                                                                              FVM_SttBackend                                        /* Receive actual Speech/AVFoundation callbacks only after preparation. */
                                                                                                                  backend;                                          /* Receive actual Speech/AVFoundation callbacks only after preparation. */
                                                                                                              FVM_SttSession *session =                             /* Own capture only after successful explicit opening. */
                                                                                                                  0;                                                /* Own capture only after successful explicit opening. */
                                                                                                              FVM_Result result = fvm_stt_system_backend(           /* Select the compiled real macOS provider. */
                                                                                                                                                         &backend); /* Select the compiled real macOS provider. */
                                                                                                              if (result == FVM_OK)                                 /* Explicitly start authorized real microphone recognition. */
                                                                                                              {                                                     /* Begin conditional branch: Explicitly start authorized real microphone recognition. */
                                                                                                                  result = fvm_stt_open(                            /* Explicitly start authorized real microphone recognition. */
                                                                                                                                        &backend,                   /* Explicitly start authorized real microphone recognition. */
                                                                                                                                        &session);                  /* Explicitly start authorized real microphone recognition. */
                                                                                                              } /* End conditional branch: Explicitly start authorized real microphone recognition. */
                                                                                                              ready(                 /* Transfer successful native-session ownership to the prepared host callback. */
                                                                                                                    result, session, /* Transfer successful native-session ownership to the prepared host callback. */
                                                                                                                    userdata);       /* Transfer successful native-session ownership to the prepared host callback. */
                                                                                                           });                       /* Finish scheduling prepared native opening. */
                                                         }];                                                                         /* Finish actual microphone permission request. */
        }];                                                                                                                          /* Finish actual speech permission request. */
    return FVM_OK;                                                                                                                   /* Report scheduling only; the callback reports actual authorization/open outcome later. */
} /* Finish explicit macOS preparation. */


/* Open actual system TTS without requesting microphone or recognition permissions. */
FVM_Result example_macos_open_tts(FVM_TtsSession **out) /* Host owns the returned voice session until explicit close. */
{                                                       /* Begin main-thread voice preparation. */
    if (!out)                                           /* Require writable session storage. */
    {                                                   /* Begin conditional branch: Require writable session storage. */
        return FVM_INVALID_ARGUMENT;                    /* Require writable session storage. */
    } /* End conditional branch: Require writable session storage. */
    *out = 0;                        /* Clear stale ownership before thread/provider checks. */
    if (![NSThread isMainThread])    /* Keep native voice lifecycle under the host UI thread. */
    {                                /* Begin conditional branch: Keep native voice lifecycle under the host UI thread. */
        return FVM_INVALID_ARGUMENT; /* Keep native voice lifecycle under the host UI thread. */
    } /* End conditional branch: Keep native voice lifecycle under the host UI thread. */
    FVM_TtsBackend backend;                               /* Receive real native Speech Synthesis callbacks. */
    FVM_Result result = fvm_tts_system_backend(&backend); /* Bind actual system voice APIs without speaking yet. */
    return result == FVM_OK ? fvm_tts_open(&backend, out) /* Transfer a real native voice or preserve actual setup failure. */
                            : result;                     /* Transfer a real native voice or preserve actual setup failure. */
} /* Finish explicit macOS TTS preparation. */
