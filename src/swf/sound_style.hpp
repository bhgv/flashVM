/* FlashVM: shared fixed-clock conversion and envelope interpolation for full and incremental voices. */
#ifndef FVM_SWF_SOUND_STYLE_HPP
#define FVM_SWF_SOUND_STYLE_HPP /* Guard shared allocation-free sound style arithmetic. */
#include "../details/vector.hpp"
#include "sound_constants.h"
/* Convert fixed-clock UI32 control positions without overflowing a 32-bit unsigned long. */
inline unsigned long fvm_swf_sound_position(unsigned long position, unsigned int rate)
{
    return (position / FVM_SWF_SOUND_CONTROL_RATE) * rate +
           ((position % FVM_SWF_SOUND_CONTROL_RATE) * rate) / FVM_SWF_SOUND_CONTROL_RATE;
}

/* Interpolate one channel's unity-scaled gain at elapsed voice time in the fixed envelope clock. */
inline double fvm_swf_sound_gain(const fvm_detail::vector<FVM_SWFSoundEnvelope> &points, size_t &segment,
                                 double position, unsigned int channel, unsigned int channels)
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
#endif
