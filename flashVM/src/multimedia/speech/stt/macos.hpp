/* OS: macOS 10.15+. Private Objective-C++ Speech/AVFoundation ownership. */
#ifndef FVM_STT_MACOS_HPP /* Protect the private Apple recognizer interface. */
#define FVM_STT_MACOS_HPP /* Define private include protection. */
#include "../session.hpp" /* Share C89-compatible native provider callbacks. */
#if defined(__APPLE__) && defined(FVM_HAS_MACOS_STT) /* Require the explicitly compiled real Apple speech framework. */
/* Open an authorized native microphone recognition task on the main thread. */
FVM_Result fvm_macos_stt_open(void *, void **); /* OS: macOS Speech/AVFoundation. */
/* Query/copy one native Unicode transcript without restarting capture. */
FVM_Result fvm_macos_stt_poll(void *, char *, size_t, size_t *, int *); /* OS: macOS Speech framework. */
/* Stop capture, cancel recognition and release native ownership on the main thread. */
void fvm_macos_stt_close(void *); /* OS: macOS Speech/AVFoundation. */
#ifdef __OBJC__ /* Hide Objective-C class syntax from ordinary C++ compilation. */
#import <AVFoundation/AVFoundation.h> /* Capture actual microphone PCM through AVAudioEngine. */
#import <Foundation/Foundation.h> /* Use real native strings, locks and ownership. */
#import <Speech/Speech.h> /* Use actual SFSpeechRecognizer and recognition task APIs. */
#define FVM_MACOS_STT_TAP_FRAMES 1024 /* Request a bounded native microphone tap buffer. */
@interface FVMAppleStt : NSObject /* Own one real recognition/capture lifecycle. */
{ /* Begin native object storage. */
    NSLock *lock; /* Protect callback publication and retained transcript snapshots. */
    SFSpeechRecognizer *recognizer; /* Own the configured system-language recognition provider. */
    SFSpeechAudioBufferRecognitionRequest *request; /* Own actual captured-audio recognition input. */
    SFSpeechRecognitionTask *task; /* Own the active native recognition task. */
    AVAudioEngine *engine; /* Own the real microphone capture graph. */
    NSString *queued; /* Own the latest actual unconsumed callback transcript. */
    NSString *pending; /* Retain one queried transcript until successful host copy. */
    BOOL queuedFinal; /* Pair finality with queued text. */
    BOOL pendingFinal; /* Preserve finality across native size queries. */
    BOOL failed; /* Preserve native error until close. */
    BOOL closing; /* Block new publications while resources are being released. */
    BOOL tapped; /* Track whether the native audio tap must be removed. */
} /* Finish native object state. */
- (FVM_Result)start; /* Explicitly activate already-authorized real recognition. */
- (FVM_Result)copyText:(char *)text /* Query/copy a retained UTF-8 result. */
              capacity:(size_t)capacity /* Query/copy a retained UTF-8 result. */
              required:(size_t *)required /* Query/copy a retained UTF-8 result. */
                 final:(int *)final; /* Query/copy a retained UTF-8 result. */
- (void)close; /* Stop/cancel actual native capture and recognition. */
@end /* Finish private recognizer interface. */
#endif /* Finish Objective-C-only declarations. */
#endif /* Finish compiled Apple speech declarations. */
#endif /* Finish private include protection. */
