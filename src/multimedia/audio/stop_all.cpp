/* OS: host-selected audio outputs. AVM1 StopSounds stops this context's active registrations. */
#include "channel.hpp" /* Import the callback barrier and context registry. */
/* Attempt every active output once, preserving the first host failure for the caller. */
FVM_Result fvm_audio_stop_all(FVM_Context *context) /* Independent contexts and standalone outputs are unaffected. */
{ /* Begin context-wide stopping. */
    if (!fvm_audio_context_valid(context)) /* Disallow registry mutation or nested stopping from a callback. */
        return FVM_INVALID_ARGUMENT; /* Never enter a callback from an invalid phase. */
    FVM_Result first = FVM_OK; /* Preserve the earliest failure in registration order. */
    FVM_AudioCallbackGuard guard(context); /* [GC:] Keep all native registrations stable across host callbacks. */
    for (size_t i = 0; i < context->audio_channels.size(); ++i) /* Attempt the complete stable channel set. */
    { /* Inspect one independent output. */
        FVM_AudioChannel *channel = context->audio_channels[i]; /* Borrow this context-owned registration. */
        if (!channel->active) /* Skip outputs with no acknowledged pending playback. */
            continue; /* Do not issue redundant stops. */
        FVM_Result result = channel->output.stop(channel->output.userdata); /* Invoke actual output stopping. */
        if (result == FVM_OK) /* Keep failed outputs active for an explicit retry. */
            channel->active = false; /* Record the acknowledged stop. */
        else if (first == FVM_OK) /* Do not overwrite an earlier independent failure. */
            first = result; /* Continue stopping the remaining outputs. */
    } /* Finish all registered attempts. */
    return first; /* Empty contexts and entirely successful stopping return OK. */
} /* Finish context-wide stopping. */
