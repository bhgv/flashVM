/* OS: host-selected PCM output. Route actual submission through a context registration. */
#include "channel.hpp" /* Import the frame validator and callback barrier. */
/* Mark playback active only after the host successfully accepts a validated PCM frame. */
FVM_Result fvm_audio_channel_write(FVM_AudioChannel *channel, const FVM_MediaFrame *frame) /* Borrow frame bytes synchronously. */
{ /* Begin registered PCM submission. */
    if (!channel || !fvm_audio_context_valid(channel->context) || !fvm_media_frame_valid(frame, FVM_MEDIA_AUDIO)) /* Validate all bounds before callbacks. */
        return FVM_INVALID_ARGUMENT; /* Leave channel state unchanged. */
    FVM_AudioCallbackGuard guard(channel->context); /* [GC:] Retain the native barrier and stable registry during output. */
    FVM_Result result = channel->output.write(channel->output.userdata, frame); /* Invoke the real host output operation. */
    if (result == FVM_OK) /* Only acknowledged submission changes the active flag. */
        channel->active = true; /* Include this output in subsequent StopSounds operations. */
    return result; /* Return the host status without replay or conversion. */
} /* Finish registered PCM submission. */
