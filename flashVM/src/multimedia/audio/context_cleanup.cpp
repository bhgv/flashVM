/* OS: host-selected audio outputs. Final teardown has no error-return channel. */
#include "channel.hpp" /* Import copied host callbacks and native registration ownership. */


/* Best-effort stop each active output once, then invalidate every context-owned handle. */
void fvm_audio_context_cleanup(FVM_Context *context) /* Called only after the context enters destruction. */


{                                                                                  /* Begin terminal output cleanup. */
    FVM_AudioCallbackGuard guard(context);                                         /* [GC:] Protect borrowed host callbacks during terminal stopping. */
    for (size_t i = 0; i < context->audio_channels.size(); ++i)                    /* Visit every owned registration once. */
    {                                                                              /* Release one output wrapper after its final stop attempt. */
        FVM_AudioChannel *channel = context->audio_channels[i];                    /* Borrow the still-live native wrapper. */
        if (channel->active)                                                       /* Never stop an output which did not acknowledge playback. */
        {                                                                          /* Begin conditional branch: Never stop an output which did not acknowledge playback. */
            fvm_audio_invoke_stop(channel->output.stop, channel->output.userdata); /* Ignore terminal errors; hosts needing status must stop explicitly first. */
        } /* End conditional branch: Never stop an output which did not acknowledge playback. */
        delete channel; /* Invalidate the wrapper without freeing borrowed host state. */
    } /* Finish terminal output release. */
    context->audio_channels.clear();                                     /* Leave no native registrations in the context being destroyed. */
    for (size_t i = 0; i < context->audio_stop_bindings.size(); ++i)     /* Release prepared-output controls after channel cleanup. */
    {                                                                    /* Attempt terminal stop once even if there were no accepted PCM writes. */
        FVM_AudioStopBinding *binding = context->audio_stop_bindings[i]; /* Borrow the live host registration. */
        fvm_audio_invoke_stop(binding->stop, binding->userdata);         /* Contain errors; teardown has no result channel. */
        delete binding;                                                  /* Invalidate the handle, leaving userdata ownership with the host. */
    } /* Finish terminal stop binding release. */
    context->audio_stop_bindings.clear(); /* Retain no native control pointer after destruction. */
} /* Finish terminal output cleanup. */
