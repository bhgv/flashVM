/* OS: host-selected outputs. Stop every StartSound voice bound to one root resource ID. */
#include "channel.hpp" /* Import the stable registry and native callback barrier. */


/* Preserve the first error while attempting every active matching voice exactly once. */
FVM_Result fvm_swf_sound_stop_bound(FVM_Context *context, unsigned int sound_id) /* Bindings remain available for explicit failed-stop retries. */
{                                                                                /* Begin resource-wide stopping. */
    if (!fvm_audio_context_valid(context) || !sound_id)                          /* Reject callback mutation, terminal phases and an absent resource identity. */
    {                                                                            /* Begin conditional branch: Reject callback mutation, terminal phases and an absent resource identity. */
        return FVM_INVALID_ARGUMENT;                                             /* Enter no callback for invalid ownership state. */
    } /* End conditional branch: Reject callback mutation, terminal phases and an absent resource identity. */


    FVM_Result first = FVM_OK;                                  /* Keep the first native failure in registration order. */
    FVM_AudioCallbackGuard guard(context);                      /* [GC:] Retain the native barrier and immutable channel registry across every callback. */
    for (size_t i = 0; i < context->audio_channels.size(); ++i) /* Consider every voice in this context, including overlap instances. */
    {                                                           /* Select one independently acknowledged output. */
        FVM_AudioChannel *voice = context->audio_channels[i];   /* Borrow a live registration protected by the guard. */
        if (!voice->active || voice->swf_sound_id != sound_id)  /* Ignore inactive, unbound and other-resource outputs. */
        {                                                       /* Begin conditional branch: Ignore inactive, unbound and other-resource outputs. */
            continue;                                           /* Preserve unrelated playback and its host callbacks. */
        } /* End conditional branch: Ignore inactive, unbound and other-resource outputs. */
        FVM_Result result = voice->output.stop(voice->output.userdata); /* Invoke the actual host discard/stop operation. */
        if (result == FVM_OK)                                           /* Commit only an acknowledged native stop. */
        {                                                               /* Begin conditional branch: Commit only an acknowledged native stop. */
            voice->active = false;                                      /* Preserve binding while allowing a later explicit start. */
        } /* End conditional branch: Commit only an acknowledged native stop. */
        else if (first == FVM_OK) /* Never overwrite the earliest native failure. */
        {                         /* Begin conditional branch: Never overwrite the earliest native failure. */
            first = result;       /* Continue attempting the remaining matching voices. */
        } /* End conditional branch: Never overwrite the earliest native failure. */
    } /* Finish all matching attempts. */
    return first; /* No matching active voice is an allocation-free successful no-op. */
} /* Finish resource-wide stopping. */
