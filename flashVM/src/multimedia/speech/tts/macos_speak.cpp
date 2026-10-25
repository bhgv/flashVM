/* OS: macOS. Speak real Unicode text through a Speech Synthesis channel. */
#include "macos.hpp" /* Import native CFString and voice APIs. */
#ifdef __APPLE__     /* Isolate Apple framework calls. */


/* Create owned CFString input and release it after native submission. */
FVM_Result fvm_macos_tts_speak(void *opaque,                                                          /* Keep all calls on the host's creating speech/UI thread. */
                               const char *text)                                                      /* Keep all calls on the host's creating speech/UI thread. */
{                                                                                                     /* Begin native speech submission. */
    FVM_MacosTts *state = static_cast<FVM_MacosTts *>(opaque);                                        /* Recover the real voice channel. */
    CFStringRef string = CFStringCreateWithCString(                                                   /* Decode actual UTF-8 with Core Foundation. */
                                                   kCFAllocatorDefault, text, kCFStringEncodingUTF8); /* Decode actual UTF-8 with Core Foundation. */
    if (!string)                                                                                      /* Reject unrepresentable input rather than inventing speech. */
    {                                                                                                 /* Begin conditional branch: Reject unrepresentable input rather than inventing speech. */
        return FVM_INVALID_ARGUMENT;                                                                  /* Reject unrepresentable input rather than inventing speech. */
    } /* End conditional branch: Reject unrepresentable input rather than inventing speech. */
    OSStatus result = SpeakCFString(state->channel, string, 0); /* Enqueue actual system synthesis/playback. */
    CFRelease(string);                                          /* Release the temporary owned Unicode string. */
    return result == noErr ? FVM_OK : FVM_ERROR;                /* Report submission, not an audible-completion event. */
} /* Finish native text submission. */
#endif /* Finish Apple-only implementation. */
