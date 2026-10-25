/* OS: macOS 10.15+. Explicitly activate authorized real Speech/AVFoundation recognition. */
#include "macos.hpp" /* Import the private Objective-C++ service owner. */


/* Create and start one actual microphone recognition session. */
FVM_Result fvm_macos_stt_open(void *unused,              /* Host permissions and main-thread lifecycle are prerequisites. */
                              void **out)                /* Host permissions and main-thread lifecycle are prerequisites. */
{                                                        /* Begin explicit native opening. */
    (void)unused;                                        /* Default native language/input belong to system settings. */
    *out = 0;                                            /* Publish no partial recognizer. */
    @autoreleasepool                                     /* Bound temporary native framework objects for the host call. */
    {                                                    /* Bound temporary native framework objects for the host call. */
        FVMAppleStt *state = [[FVMAppleStt alloc] init]; /* Own actual native service state. */
        if (!state)                                      /* Avoid framework calls after allocation failure. */
        {                                                /* Begin conditional branch: Avoid framework calls after allocation failure. */
            return FVM_OUT_OF_MEMORY;                    /* Avoid framework calls after allocation failure. */
        } /* End conditional branch: Avoid framework calls after allocation failure. */
        FVM_Result result = [state start]; /* Explicitly activate permitted real microphone recognition. */
        if (result != FVM_OK)              /* Release partial startup ownership. */
        {                                  /* Release partial startup ownership. */
            [state close];                 /* Stop any tap/task successfully created before failure. */
            [state release];               /* Release the native owner after deterministic cleanup. */
            return result;                 /* Preserve permission, thread, service and capture failure categories. */
        } /* Finish failed startup cleanup. */
        *out = state;  /* Transfer retained Objective-C ownership to the common session. */
        return FVM_OK; /* Native capture remains active until final result/error or close. */
    } /* Finish temporary native ownership scope. */
} /* Finish explicit native opening. */
