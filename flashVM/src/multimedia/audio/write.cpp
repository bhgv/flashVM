/* OS: host-selected output library. No implicit device, thread or queue. */
#include "../common/session.hpp" /* Reuse overflow-safe buffer validation. */


/* Consume the borrowed audio frame synchronously in the host's system adapter. */
FVM_Result fvm_audio_write(const FVM_MediaOutput *output,                      /* Do not take ownership of either argument. */
                           const FVM_MediaFrame *frame)                        /* Do not take ownership of either argument. */
{                                                                              /* Begin explicit host output. */
    if (!output || output->struct_size != sizeof(*output) || !output->write || /* Validate the ABI and all buffer bounds. */
        !fvm_media_frame_valid(frame, FVM_MEDIA_AUDIO))                        /* Validate the ABI and all buffer bounds. */
    {                                                                          /* Begin scope: Validate the ABI and all buffer bounds. */
        return FVM_INVALID_ARGUMENT;                                           /* Validate the ABI and all buffer bounds. */
    } /* End scope: Validate the ABI and all buffer bounds. */
    return output->write(output->userdata, /* Return the result produced by the following backend operation. */
                         frame);           /* Delegate playback or presentation to the real host implementation. */
} /* Finish the output adapter. */
