/* OS: portable audio registration state query. No host code is invoked. */
#include "channel.hpp" /* Import the context-owned active flag. */
/* Permit diagnostics during an output callback while rejecting collection/destruction. */
FVM_Result fvm_audio_channel_active(FVM_AudioChannel *channel, int *out) /* Copy one Boolean state without ownership transfer. */
{ /* Begin the read-only query. */
    if (!channel || !out || channel->context->destroying || channel->context->gc_collecting || channel->context->gc_releasing) /* Validate the live owner. */
        return FVM_INVALID_ARGUMENT; /* Leave output storage unchanged on failure. */
    *out = channel->active ? 1 : 0; /* Publish only the last acknowledged submission/stop state. */
    return FVM_OK; /* No allocation, callback or collection occurs. */
} /* Finish the state query. */
