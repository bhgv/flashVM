/* OS: macOS 10.15+. Real Speech-framework recognition with explicit authorized microphone capture. */
#include "macos.hpp" /* Import Objective-C++ native ownership, not public ABI types. */
#include <string.h> /* Copy actual recognized UTF-8 into bounded caller storage. */
@implementation FVMAppleStt /* Implement the real native-service owner under manual reference counting. */
/* Initialize ownership only; microphone activation belongs to start. */
- (id)init /* Keep construction separate from permission-sensitive capture. */
{ /* Begin native object initialization. */
    self = [super init]; /* Initialize NSObject ownership. */
    if (self) /* Own a lock before any asynchronous callbacks exist. */
        lock = [[NSLock alloc] init]; /* Own a lock before any asynchronous callbacks exist. */
    return self; /* Return ordinary retained Objective-C ownership. */
} /* Finish object initialization. */
/* Start only on the main thread after host-obtained microphone and speech permissions. */
- (FVM_Result)start /* This explicit call is the native capture boundary. */
{ /* Begin actual native recognition. */
    if (![NSThread isMainThread]) /* Require host main-thread lifecycle control. */
        return FVM_INVALID_ARGUMENT; /* Require host main-thread lifecycle control. */
    if ([SFSpeechRecognizer authorizationStatus] != SFSpeechRecognizerAuthorizationStatusAuthorized || /* Never silently request permissions or open unauthorized capture. */
        [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio] != AVAuthorizationStatusAuthorized) /* Never silently request permissions or open unauthorized capture. */
        return FVM_UNSUPPORTED; /* Never silently request permissions or open unauthorized capture. */
    recognizer = [[SFSpeechRecognizer alloc] init]; /* Use the installed system-language recognizer. */
    if (!recognizer || !recognizer.available) /* Preserve missing/unavailable real recognition service. */
        return FVM_UNSUPPORTED; /* Preserve missing/unavailable real recognition service. */
    request = [[SFSpeechAudioBufferRecognitionRequest alloc] init]; /* Own real captured-audio input for recognition. */
    engine = [[AVAudioEngine alloc] init]; /* Own the actual native microphone graph. */
    if (!request || !engine || !lock) /* Do not activate capture with incomplete ownership. */
        return FVM_OUT_OF_MEMORY; /* Do not activate capture with incomplete ownership. */
    request.shouldReportPartialResults = YES; /* Permit useful actual partial hypotheses. */
    AVAudioInputNode *input = engine.inputNode; /* Borrow the actual native capture input node. */
    AVAudioFormat *format = [input inputFormatForBus:0]; /* Read the device's real negotiated PCM format. */
    if (!format || format.sampleRate <= 0 || !format.channelCount) /* Reject an unavailable or invalid microphone format. */
        return FVM_ERROR; /* Reject an unavailable or invalid microphone format. */
    SFSpeechAudioBufferRecognitionRequest *capturedRequest = /* Let the copied tap block retain request independently of close. */
        request; /* Let the copied tap block retain request independently of close. */
    [input /* Continue the same native operation with the listed typed arguments. */
        installTapOnBus:0 /* Continue the same native operation with the listed typed arguments. */
             bufferSize:FVM_MACOS_STT_TAP_FRAMES /* Continue the same native operation with the listed typed arguments. */
                 format:format /* Continue the same native operation with the listed typed arguments. */
                  block:^( /* Connect the real microphone PCM stream to the recognizer. */
                      AVAudioPCMBuffer *buffer, /* Connect the real microphone PCM stream to the recognizer. */
                      AVAudioTime * /* Connect the real microphone PCM stream to the recognizer. */
                          when) { /* Connect the real microphone PCM stream to the recognizer. */
                                  (void) /* Native timestamps remain under the system recognition provider's control. */
                                      when; /* Native timestamps remain under the system recognition provider's control. */
                                  [capturedRequest /* Feed actual captured speech, not synthesized fixture audio. */
                                      appendAudioPCMBuffer: /* Feed actual captured speech, not synthesized fixture audio. */
                                          buffer]; /* Feed actual captured speech, not synthesized fixture audio. */
                  }]; /* Finish installing the native capture tap. */
    tapped = YES; /* Ensure partial-failure cleanup removes the real tap. */
    task = [[recognizer /* Continue the same native operation with the listed typed arguments. */
        recognitionTaskWithRequest:request /* Continue the same native operation with the listed typed arguments. */
                     resultHandler:^( /* Receive actual asynchronous recognition results. */
                         SFSpeechRecognitionResult *result, /* Receive actual asynchronous recognition results. */
                         NSError * /* Receive actual asynchronous recognition results. */
                             error) { /* Receive actual asynchronous recognition results. */
                                      [lock lock]; /* Serialize publication with native polling and close. */
                                      if (!closing) /* Ignore late callbacks after capture shutdown begins. */
                                      { /* Ignore late callbacks after capture shutdown begins. */
                                          if (result) /* Publish only actual system transcripts. */
                                          { /* Publish only actual system transcripts. */
                                              [queued /* Replace the previous unconsumed hypothesis with the newest one. */
                                                  release]; /* Replace the previous unconsumed hypothesis with the newest one. */
                                              queued = [result.bestTranscription.formattedString /* Own stable recognized Unicode text. */
                                                            copy]; /* Own stable recognized Unicode text. */
                                              queuedFinal = /* Pair finality atomically with that transcript. */
                                                  result.final; /* Pair finality atomically with that transcript. */
                                          } /* Finish transcript publication. */
                                          if (error) /* Preserve a real provider/network/capture failure. */
                                              failed = YES; /* Preserve a real provider/network/capture failure. */
                                      } /* Finish active-session publication. */
                                      [lock unlock]; /* Release publication synchronization promptly. */
                     }] retain]; /* Own the real recognition task until close. */
    if (!task) /* Do not report successful recognition without a native task. */
        return FVM_ERROR; /* Do not report successful recognition without a native task. */
    [engine prepare]; /* Prepare the real capture graph before activation. */
    NSError *error = nil; /* Receive actual microphone startup failure. */
    if (![engine startAndReturnError:&error]) /* Explicitly activate real microphone capture. */
        return FVM_ERROR; /* Explicitly activate real microphone capture. */
    return FVM_OK; /* Recognition continues until final result/error or explicit close. */
} /* Finish actual recognition startup. */
/* Keep a stable pending transcript across callback replacement and size queries. */
- (FVM_Result)copyText:(char *)text /* Borrow host output storage only during this method. */
              capacity:(size_t)capacity /* Borrow host output storage only during this method. */
              required:(size_t *)required /* Borrow host output storage only during this method. */
                 final:(int *)final /* Borrow host output storage only during this method. */
{ /* Begin nonblocking native text retrieval. */
    [lock lock]; /* Protect queued/pending ownership from asynchronous callbacks. */
    if (!pending && queued) /* Snapshot the next actual transcript only once. */
    { /* Snapshot the next actual transcript only once. */
        pending = queued; /* Transfer owned Unicode text from callback queue to query/copy state. */
        queued = nil; /* Leave the callback free to publish another independent hypothesis. */
        pendingFinal = queuedFinal; /* Preserve the exact snapshot's finality. */
    } /* Finish stable snapshot selection. */
    FVM_Result result = /* Distinguish empty queue from a real native service failure. */
        failed ? FVM_ERROR : FVM_NOT_FOUND; /* Distinguish empty queue from a real native service failure. */
    if (pending) /* Prefer delivering existing recognized text even if a later error occurred. */
    { /* Prefer delivering existing recognized text even if a later error occurred. */
        const char *bytes = /* Borrow actual standard UTF-8 while the owned NSString stays locked/alive. */
            [pending UTF8String]; /* Borrow actual standard UTF-8 while the owned NSString stays locked/alive. */
        if (bytes) /* Publish only successfully converted native Unicode. */
        { /* Publish only successfully converted native Unicode. */
            *required = strlen(bytes) + 1; /* Report exact caller capacity including NUL. */
            *final = pendingFinal ? 1 : 0; /* Preserve final/partial state paired with the text. */
            result = FVM_OK; /* Query mode can return success without consuming pending ownership. */
            if (capacity && capacity < *required) /* Retain the snapshot for a later sufficient buffer. */
                result = FVM_INVALID_ARGUMENT; /* Retain the snapshot for a later sufficient buffer. */
            else if (capacity) /* Copy only when a complete result fits. */
            { /* Copy only when a complete result fits. */
                memcpy(text, bytes, *required); /* Transfer actual recognized UTF-8 to host-owned storage. */
                [pending release]; /* Release only after the host owns a complete copy. */
                pending = nil; /* Permit selecting the next queued transcript. */
            } /* Finish successful native text copying. */
        } /* Preserve an actual Unicode conversion/allocation failure. */
        else /* Preserve an actual Unicode conversion/allocation failure. */
            result = FVM_ERROR; /* Preserve an actual Unicode conversion/allocation failure. */
    } /* Finish pending transcript handling. */
    [lock unlock]; /* Let asynchronous publication continue. */
    return result; /* Return actual query/copy/provider status. */
} /* Finish native text retrieval. */
/* Stop capture first, then cancel and release recognition resources. */
- (void)close /* Called on the creating host main thread. */
{ /* Begin actual native recognition shutdown. */
    [lock lock]; /* Synchronize shutdown state with pending callbacks. */
    BOOL already = closing; /* Make helper cleanup idempotent. */
    closing = YES; /* Block late publications immediately. */
    [lock unlock]; /* Do not hold a callback lock while cancelling native services. */
    if (already) /* Avoid duplicate native tap removal and resource release. */
        return; /* Avoid duplicate native tap removal and resource release. */
    [engine stop]; /* Stop actual microphone capture. */
    if (tapped) /* Release the copied tap block and its independent request reference. */
        [engine.inputNode removeTapOnBus:0]; /* Release the copied tap block and its independent request reference. */
    tapped = NO; /* Mark the capture tap removed. */
    [request endAudio]; /* Signal actual captured-audio input completion. */
    [task cancel]; /* Cancel pending recognition work without restarting capture. */
    [task release]; /* Release this object's native task ownership. */
    task = nil; /* Do not retain a task/self-block cycle after explicit close. */
    [engine release]; /* Release the real capture graph. */
    engine = nil; /* Mark native capture ownership released. */
    [request release]; /* Release the owned recognition input request. */
    request = nil; /* Mark native input ownership released. */
    [recognizer release]; /* Release the actual recognition-service object. */
    recognizer = nil; /* Mark native service ownership released. */
} /* Finish actual native shutdown. */
/* Release retained text and synchronization after all copied callback blocks release this object. */
- (void)dealloc /* Manual Objective-C ownership is independent of the VM collector. */
{ /* Begin final native owner cleanup. */
    [self close]; /* Handle failed/partial initialization consistently. */
    [queued release]; /* Release the final unconsumed actual hypothesis. */
    [pending release]; /* Release a transcript retained after a size query. */
    [lock release]; /* Release native synchronization after callbacks no longer own this receiver. */
    [super dealloc]; /* Finish manual Objective-C object ownership. */
} /* Finish final owner cleanup. */
@end /* Finish the real macOS recognition adapter. */
