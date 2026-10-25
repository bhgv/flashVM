/* FlashVM: shared fixed-clock conversion and envelope interpolation for full and incremental voices. */
#ifndef FVM_SWF_SOUND_STYLE_HPP
#define FVM_SWF_SOUND_STYLE_HPP /* Guard shared allocation-free sound style arithmetic. */
#include "../details/vector.hpp"
#include "sound_constants.h"
#include <climits>


/* Convert fixed-clock UI32 control positions without overflowing a 32-bit unsigned long. */
inline FVM_Result fvm_swf_sound_position(unsigned long position, unsigned int rate, unsigned long *out)
{
    if (!out || !rate)
    {
        return FVM_INVALID_ARGUMENT;
    }
    unsigned long whole = position / FVM_SWF_SOUND_CONTROL_RATE;
    unsigned long remainder = position % FVM_SWF_SOUND_CONTROL_RATE;
    /* Split both operands: each fractional product is below the native unsigned-long bound. */
    unsigned long fraction = remainder * (rate / FVM_SWF_SOUND_CONTROL_RATE) +
                             remainder * (rate % FVM_SWF_SOUND_CONTROL_RATE) / FVM_SWF_SOUND_CONTROL_RATE;
    if (whole > ULONG_MAX / rate || whole * rate > ULONG_MAX - fraction)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = whole * rate + fraction;
    return FVM_OK;
}


/* Interpolate one channel's unity-scaled gain at elapsed voice time in the fixed envelope clock. */
inline double fvm_swf_sound_gain(const fvm_detail::vector<FVM_SWFSoundEnvelope> &points,
                                 size_t &segment,
                                 double position,
                                 unsigned int channel,
                                 unsigned int channels)
{
    if (points.empty())
    {
        return FVM_SWF_SOUND_ENVELOPE_UNITY;
    }
    while (segment + 1 < points.size() && position >= points[segment + 1].position_44)
    {
        ++segment;
    }
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
