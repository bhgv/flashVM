/* OS: host-selected audio bridge. Unregister only after acknowledged output stopping. */
#include "channel.hpp" /* Import the registry layout and stopping operation. */


/* Keep a failed active output registered so the caller can retry stop or close. */
FVM_Result fvm_audio_channel_close(FVM_AudioChannel *channel) /* Null is an idempotent cleanup input. */
{                                                             /* Begin channel cleanup. */
    if (!channel)                                             /* Permit ordinary partial-setup cleanup. */
    {                                                         /* Begin conditional branch: Permit ordinary partial-setup cleanup. */
        return FVM_OK;                                        /* No registration or output exists. */
    } /* End conditional branch: Permit ordinary partial-setup cleanup. */
    FVM_Result result = fvm_audio_channel_stop(channel); /* Validate lifecycle and stop acknowledged playback first. */
    if (result != FVM_OK)                                /* Failed stopping must not orphan a live output. */
    {                                                    /* Begin conditional branch: Failed stopping must not orphan a live output. */
        return result;                                   /* Leave the handle valid for retry. */
    } /* End conditional branch: Failed stopping must not orphan a live output. */
    fvm_detail::vector<FVM_AudioChannel *> &channels = channel->context->audio_channels; /* Borrow the stable owner registry. */
    for (size_t i = 0; i < channels.size(); ++i)                                         /* Find the exact native registration. */
    {                                                                                    /* Examine one registry slot. */
        if (channels[i] == channel)                                                      /* Match pointer identity rather than shared userdata. */
        {                                                                                /* Commit successful unregistering. */
            channels.erase(channels.begin() + i);                                        /* Remove the owner reference without calling host code. */
            delete channel;                                                              /* Release the wrapper; host userdata is never freed by FlashVM. */
            return FVM_OK;                                                               /* The caller must now discard its invalid handle. */
        } /* Finish unregistering this channel. */
    } /* Finish the registry search. */
    return FVM_INVALID_ARGUMENT; /* Report an inconsistent private handle without deleting unknown storage. */
} /* Finish channel cleanup. */
