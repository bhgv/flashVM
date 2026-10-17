/* OS: host-selected audio outputs. Final teardown has no error-return channel. */
#include "channel.hpp" /* Import copied host callbacks and native registration ownership. */
/* Best-effort stop each active output once, then invalidate every context-owned handle. */
void fvm_audio_context_cleanup(FVM_Context *context) /* Called only after the context enters destruction. */
{ /* Begin terminal output cleanup. */
    FVM_AudioCallbackGuard guard(context); /* [GC:] Protect borrowed host callbacks during terminal stopping. */
    for (size_t i = 0; i < context->audio_channels.size(); ++i) /* Visit every owned registration once. */
    { /* Release one output wrapper after its final stop attempt. */
        FVM_AudioChannel *channel = context->audio_channels[i]; /* Borrow the still-live native wrapper. */
        if (channel->active) /* Never stop an output which did not acknowledge playback. */
            channel->output.stop(channel->output.userdata); /* Ignore terminal errors; hosts needing status must stop explicitly first. */
        delete channel; /* Invalidate the wrapper without freeing borrowed host state. */
    } /* Finish terminal output release. */
    context->audio_channels.clear(); /* Leave no native registrations in the context being destroyed. */
} /* Finish terminal output cleanup. */
