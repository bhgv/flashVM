/* OS: host-selected audio output. Stop one active registration with retryable failures. */
#include "channel.hpp" /* Import the copied callbacks and lifecycle checks. */
/* Avoid redundant stop callbacks for outputs with no successful submission. */
FVM_Result fvm_audio_channel_stop(FVM_AudioChannel *channel) /* Retain the handle on success and failure. */
{ /* Begin explicit channel stopping. */
    if (!channel || !fvm_audio_context_valid(channel->context)) /* Reject callback reentry and invalid context phases. */
        return FVM_INVALID_ARGUMENT; /* Preserve output and registry state. */
    if (!channel->active) /* An inactive output has nothing acknowledged for stopping. */
        return FVM_OK; /* Avoid a redundant host callback. */
    FVM_AudioCallbackGuard guard(channel->context); /* [GC:] Block collection and channel mutation through the callback. */
    FVM_Result result = channel->output.stop(channel->output.userdata); /* Stop and discard the host's pending output. */
    if (result == FVM_OK) /* A failed stop remains retryable. */
        channel->active = false; /* Remove acknowledged playback from future stop-all work. */
    return result; /* Preserve the host's exact error status. */
} /* Finish explicit stopping. */
