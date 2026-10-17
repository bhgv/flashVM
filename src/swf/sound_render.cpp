/* FlashVM: bounded StartSound voice rendering with loops and linear gain envelopes.
 * Decoder bytes are consumed before output; the returned native buffer survives SWF reload. */
#include "sound_render.hpp"
#include "sound_constants.h"
#include "sound_event_constants.h"
#include <new>

namespace
{
/* Convert fixed-clock UI32 control positions without overflowing a 32-bit unsigned long. */
unsigned long source_position(unsigned long position, unsigned int rate)
{
    return (position / FVM_SWF_SOUND_CONTROL_RATE) * rate +
           ((position % FVM_SWF_SOUND_CONTROL_RATE) * rate) / FVM_SWF_SOUND_CONTROL_RATE;
}

/* Interpolate one channel's unity-scaled gain at elapsed voice time in the fixed envelope clock. */
double sound_gain(const fvm_detail::vector<FVM_SWFSoundEnvelope> &points, size_t &segment, double position,
                  unsigned int channel, unsigned int channels)
{
    if (points.empty())
        return FVM_SWF_SOUND_ENVELOPE_UNITY;
    while (segment + 1 < points.size() && position >= points[segment + 1].position_44)
        ++segment;
    const FVM_SWFSoundEnvelope &left = points[segment];
    double gain = channels == FVM_SWF_SOUND_MONO_CHANNELS
                      ? ((double)left.left_level + left.right_level) / FVM_SWF_SOUND_STEREO_CHANNELS
                      : (channel ? left.right_level : left.left_level);
    if (position > left.position_44 && segment + 1 < points.size())
    {
        const FVM_SWFSoundEnvelope &right = points[segment + 1];
        double end = channels == FVM_SWF_SOUND_MONO_CHANNELS
                         ? ((double)right.left_level + right.right_level) / FVM_SWF_SOUND_STEREO_CHANNELS
                         : (channel ? right.right_level : right.left_level);
        gain += (end - gain) * ((position - left.position_44) / (double)(right.position_44 - left.position_44));
    }
    return gain;
}
} // namespace

/* Decode the selected source interval once, then repeat it with an elapsed-time envelope. */
FVM_Result fvm_swf_sound_render(FVM_Context *context, size_t ordinal, const FVM_SWFSoundEvent &event,
                                const FVM_SWFSoundInfo &info, fvm_detail::vector<short> &output)
{
    unsigned long first =
        event.flags & FVM_SWF_SOUND_EVENT_IN_POINT ? source_position(event.in_point, info.sample_rate) : 0;
    unsigned long end = info.sample_count;
    if (event.flags & FVM_SWF_SOUND_EVENT_OUT_POINT)
    {
        unsigned long last = source_position(event.out_point, info.sample_rate);
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
                double gain = sound_gain(points, segment, position, channel, info.channels);
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
