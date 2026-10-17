/* OS: macOS 10.15+. Stop actual microphone capture and release native recognition ownership. */
#include "macos.hpp" /* Import the private recognition owner. */
/* Close on the host's main thread; copied callbacks retain their receiver until they finish. */
void fvm_macos_stt_close(void *opaque) /* Common session close invokes this exactly once. */
{ /* Begin native service cleanup. */
    @autoreleasepool /* Bound framework cleanup temporaries. */
    { /* Bound framework cleanup temporaries. */
        FVMAppleStt *state = static_cast<FVMAppleStt *>(opaque); /* Recover owned native service state. */
        [state close]; /* Stop capture, remove its tap and cancel actual recognition. */
        [state release]; /* Release the common session's retained native ownership. */
    } /* Finish native temporary ownership scope. */
} /* Finish native service cleanup. */
