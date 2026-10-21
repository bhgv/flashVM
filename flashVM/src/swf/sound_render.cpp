/* FlashVM: bounded StartSound voice rendering with loops and linear gain envelopes.
 * Decoder bytes are consumed before output; the returned native buffer survives SWF reload. */
#include "sound_render.hpp"
#include "sound_constants.h"
#include "sound_event_constants.h"
#include "sound_style.hpp"
#include <new>

/* Decode the selected source interval once, then repeat it with an elapsed-time envelope. */
FVM_Result fvm_swf_sound_render(FVM_Context *context, size_t ordinal, const FVM_SWFSoundEvent &event,
                                const FVM_SWFSoundInfo &info, fvm_detail::vector<short> &output)
{
    unsigned long first = 0;
    if (event.flags & FVM_SWF_SOUND_EVENT_IN_POINT)
    {
        FVM_Result position_result = fvm_swf_sound_position(event.in_point, info.sample_rate, &first);
        if (position_result != FVM_OK)
            return position_result;
    }
    unsigned long end = info.sample_count;
    if (event.flags & FVM_SWF_SOUND_EVENT_OUT_POINT)
    {
        unsigned long last = 0;
        FVM_Result position_result = fvm_swf_sound_position(event.out_point, info.sample_rate, &last);
        if (position_result != FVM_OK)
            return position_result;
        if (last >= info.sample_count)
            return FVM_INVALID_ARGUMENT;
        end = last + 1;
    }
    if (first > end)
        return FVM_INVALID_ARGUMENT;
    unsigned int loops = event.flags & FVM_SWF_SOUND_EVENT_LOOPS ? event.loop_count : FVM_SWF_SOUND_EVENT_DEFAULT_PLAYS;
    size_t checked = 0;
    FVM_Result result = fvm_swf_sound_read_pcm(context, event.sound_id, first, 0, 0, 0, &checked);
    if (result != FVM_OK)
        return result;
    size_t limit = context->runtime->config.memory_limit_bytes ? context->runtime->config.memory_limit_bytes
                                                               : FVM_SWF_DEFAULT_LIMIT;
    size_t envelope_bytes = event.envelope_count * sizeof(FVM_SWFSoundEnvelope);
    if (envelope_bytes > limit)
        return FVM_OUT_OF_MEMORY;
    size_t available = (limit - envelope_bytes) / sizeof(short) / info.channels;
    unsigned long source_frames = end - first;
    /* Both decoded source and repeated output must fit, without overflowing intermediate products. */
    if (loops && source_frames > available / (loops + 1UL))
        return FVM_OUT_OF_MEMORY;
    size_t frames = loops ? (size_t)source_frames : 0;
    try
    {
        fvm_detail::vector<FVM_SWFSoundEnvelope> points(event.envelope_count);
        for (size_t i = 0; i < points.size(); ++i)
        {
            points[i].struct_size = sizeof(points[i]);
            result = fvm_swf_sound_event_envelope(context, ordinal, i, &points[i]);
            if (result != FVM_OK)
                return result;
            if (i && points[i].position_44 <= points[i - 1].position_44)
                return FVM_INVALID_ARGUMENT;
        }
        fvm_detail::vector<short> source(frames * info.channels);
        result = fvm_swf_sound_read_pcm(context, event.sound_id, first, frames, source.empty() ? 0 : source.begin(),
                                        source.size(), &checked);
        if (result != FVM_OK)
            return result;
        fvm_detail::vector<short> rendered(frames * loops * info.channels);
        size_t segment = 0;
        for (size_t frame = 0; frame < frames * loops; ++frame)
        {
            double position = (double)frame * FVM_SWF_SOUND_ENVELOPE_RATE / info.sample_rate;
            for (unsigned int channel = 0; channel < info.channels; ++channel)
            {
                double gain = fvm_swf_sound_gain(points, segment, position, channel, info.channels);
                double value = source[(frame % frames) * info.channels + channel] * gain / FVM_SWF_SOUND_ENVELOPE_UNITY;
                if (value < FVM_SWF_SOUND_SHORT_MIN)
                    value = FVM_SWF_SOUND_SHORT_MIN;
                if (value > FVM_SWF_SOUND_SHORT_MAX)
                    value = FVM_SWF_SOUND_SHORT_MAX;
                rendered[frame * info.channels + channel] = (short)value;
            }
        }
        output.swap(rendered);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
