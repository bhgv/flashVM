/* OS: host-selected output. Apply one root StartSound to a dedicated single-voice channel. */
#include "channel.hpp" /* Reuse channel lifecycle, ownership and guarded real output callbacks. */
#include "../../swf/sound_render.hpp" /* Prepare copied looped/enveloped PCM before host callbacks. */
#include <cstring> /* Initialize unused output geometry without exposing uninitialized fields. */
/* Apply synchronization to this channel only; hosts allocate separate channels for overlapping voices. */
FVM_Result fvm_swf_sound_event_submit(FVM_Context *context, size_t ordinal, FVM_AudioChannel *channel) /* Borrow the channel until the synchronous operation returns. */
{ /* Begin explicit StartSound application. */
    if (!channel || channel->context != context || !fvm_audio_context_valid(context)) /* Reject foreign owners, terminal phases and callback mutation. */
        return FVM_INVALID_ARGUMENT; /* Preserve channel state without entering output. */
    FVM_SWFSoundEvent event; /* Receive copied controls from the current canonical SWF. */
    event.struct_size = sizeof(event); /* Declare the exact event inspection ABI. */
    FVM_Result result = fvm_swf_sound_event_at(context, ordinal, &event); /* Validate all controls and resolve the selected unique sound. */
    if (result != FVM_OK) /* Preserve malformed and missing resource results. */
        return result; /* Do not change binding or output on inspection failure. */
    if ((channel->swf_sound_id && channel->swf_sound_id != event.sound_id) || (!channel->swf_sound_id && channel->active)) /* Never stop or suppress a foreign or unbound active voice. */
        return FVM_INVALID_ARGUMENT; /* Require a dedicated channel for the selected resource. */
    if (event.flags & FVM_SWF_SOUND_EVENT_STOP) /* Give an explicit stop priority over all playback styles. */
        return fvm_audio_channel_stop(channel); /* [GC:] Use the ordinary retryable, guarded native stop callback. */
    if (channel->active) /* Acknowledged output remains active until the host explicitly stops it. */
        return event.flags & FVM_SWF_SOUND_EVENT_NO_MULTIPLE ? FVM_OK : FVM_INVALID_ARGUMENT; /* Suppress a duplicate or require a separate channel for overlap. */
    FVM_SWFSoundInfo info; /* Receive source rate/channel metadata before any host callback. */
    info.struct_size = sizeof(info); /* Declare the exact resource inspection ABI. */
    result = fvm_swf_sound_info(context, event.sound_id, &info); /* Resolve nominal geometry without opening a device. */
    if (result != FVM_OK) /* Preserve resource lookup errors. */
        return result; /* Nothing has been rendered or submitted. */
    fvm_detail::vector<short> samples; /* Own all styled PCM independently of reloadable SWF bytes. */
    result = fvm_swf_sound_render(context, ordinal, event, info, samples); /* Decode once, repeat and apply gains within the configured temporary limit. */
    if (result != FVM_OK || samples.empty()) /* Empty valid voices do not activate or bind output. */
        return result; /* Preserve style/codec/OOM errors without host side effects. */
    FVM_MediaFrame frame; /* Describe one complete rendered voice with an elapsed playback clock. */
    std::memset(&frame, 0, sizeof(frame)); /* Clear geometry fields unused by audio. */
    frame.kind = FVM_MEDIA_AUDIO; /* Select native signed-16 interleaved output. */
    frame.data = reinterpret_cast<const unsigned char *>(samples.begin()); /* Borrow native copied samples through this synchronous call. */
    frame.size = samples.size() * sizeof(short); /* Publish only initialized sample values. */
    frame.sample_rate = info.sample_rate; /* Preserve the resource's nominal source clock. */
    frame.channels = info.channels; /* Preserve mono/stereo channel layout. */
    frame.samples = samples.size() / info.channels; /* Include every requested loop in the complete voice. */
    frame.has_timestamp = 1; /* Start the new voice's elapsed clock explicitly at zero seconds. */
    result = fvm_audio_channel_write(channel, &frame); /* [GC:] Enter the existing native barrier exactly once after preparation. */
    if (result == FVM_OK) /* Failed native writes must not commit resource identity. */
        channel->swf_sound_id = event.sound_id; /* Retain dedicated resource binding through stops and reloads until close. */
    return result; /* Return the host status without replay or hidden waiting. */
} /* Finish explicit StartSound application. */
