/* OS: host-selected output bridge. Register a copied table in one VM context. */
#include "channel.hpp" /* Import lifecycle state and the public C89 API. */


/* Publish an inactive channel only after registry insertion succeeds. */
FVM_Result fvm_audio_channel_create(FVM_Context *context, const FVM_MediaOutput *output, FVM_AudioChannel **out) /* Borrow host callbacks. */
{                                                                                                                /* Begin transactional registration. */
    if (!out)                                                                                                    /* Require a destination handle. */
    {                                                                                                            /* Begin conditional branch: Require a destination handle. */
        return FVM_INVALID_ARGUMENT;                                                                             /* Avoid touching absent output storage. */
    } /* End conditional branch: Require a destination handle. */
    *out = 0;                                                                                     /* Leave a deterministic handle on every failure. */
    if (!fvm_audio_context_valid(context) || !output || output->struct_size != sizeof(*output) || /* Validate lifecycle and table size. */
        !output->write || !output->stop)                                                          /* Every registered channel must support actual submission and stopping. */
    {                                                                                             /* Begin scope: Every registered channel must support actual submission and stopping. */
        return FVM_INVALID_ARGUMENT;                                                              /* Publish nothing for an incomplete output bridge. */
    } /* End scope: Every registered channel must support actual submission and stopping. */
    FVM_AudioChannel *channel = 0;                  /* Track partial ownership for allocation failure cleanup. */
    try                                             /* Contain both channel and vector allocation failures at the C ABI. */
    {                                               /* Build an entirely initialized registration. */
        channel = new FVM_AudioChannel;             /* Allocate native storage outside the managed object heap. */
        channel->context = context;                 /* Bind the registry owner before publication. */
        channel->output = *output;                  /* Copy callbacks, retaining only the borrowed host userdata pointer. */
        channel->swf_sound_id = 0;                  /* Keep resource binding unset until successful event submission. */
        channel->active = false;                    /* Registration alone never opens or plays a device. */
        context->audio_channels.push_back(channel); /* Commit the new registration to its owner. */
        *out = channel;                             /* Publish only after insertion cannot fail. */
        return FVM_OK;                              /* Report the fully registered inactive output. */
    } /* Finish successful construction. */
    catch (const std::bad_alloc &) /* Translate native allocation failure. */
    {                              /* Undo any uncommitted registration. */
        delete channel;            /* Release only the newly allocated wrapper, never host userdata. */
        return FVM_OUT_OF_MEMORY;  /* Keep the caller's output handle null. */
    } /* Finish allocation failure cleanup. */
} /* Finish registration. */
