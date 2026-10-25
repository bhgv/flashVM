/* OS: host-selected audio library. Stop only the supplied output instance. */
#include "flashvm/multimedia.h" /* Import the host output callback contract. */


/* Request synchronous stop without inventing VM sound-channel state. */
FVM_Result fvm_audio_stop(const FVM_MediaOutput *output)   /* Leave queued-buffer release to the host implementation. */
{                                                          /* Begin stopping the selected output. */
    if (!output || output->struct_size != sizeof(*output)) /* Validate the callback-table layout. */
    {                                                      /* Begin conditional branch: Validate the callback-table layout. */
        return FVM_INVALID_ARGUMENT;                       /* Validate the callback-table layout. */
    } /* End conditional branch: Validate the callback-table layout. */
    if (!output->stop)          /* Report an absent stop operation explicitly. */
    {                           /* Begin conditional branch: Report an absent stop operation explicitly. */
        return FVM_UNSUPPORTED; /* Report an absent stop operation explicitly. */
    } /* End conditional branch: Report an absent stop operation explicitly. */
    return output->stop(output->userdata); /* Invoke the actual system/library stop bridge. */
} /* Finish audio stopping. */
