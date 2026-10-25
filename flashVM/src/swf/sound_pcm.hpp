/* FlashVM: shared bounded native-S16 conversion for DefineSound and root stream PCM. */
#ifndef FVM_SWF_SOUND_PCM_HPP
#define FVM_SWF_SOUND_PCM_HPP /* Guard shared allocation-free decoding. */
#include "sound_adpcm.hpp"
#include "sound_mp3.hpp"
#include <climits>
#include <cstring>


/* Decode a prevalidated source view after checking host representation and caller range/capacity. */
static inline FVM_Result fvm_swf_sound_decode_pcm(const FVM_SWFSoundInfo &info,
                                                  unsigned long first,
                                                  size_t frames,
                                                  short *destination,
                                                  size_t capacity,
                                                  size_t *written)
{
    if (!written)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *written = 0;
    if ((info.format != FVM_SWF_SOUND_ADPCM && info.format != FVM_SWF_SOUND_MP3 &&
         (info.format != FVM_SWF_SOUND_FORMAT_NATIVE && info.format != FVM_SWF_SOUND_FORMAT_LE)) ||
        sizeof(short) != FVM_SWF_SOUND_PCM_BYTES || SHRT_MAX != FVM_SWF_SOUND_SHORT_MAX ||
        SHRT_MIN != FVM_SWF_SOUND_SHORT_MIN)
    {
        return FVM_UNSUPPORTED;
    }
    if (first > info.sample_count || frames > info.sample_count - first || frames > capacity / info.channels ||
        (frames && !destination))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (info.format == FVM_SWF_SOUND_MP3)
    {
        return fvm_swf_mp3_read(info, first, frames, destination, capacity, written);
    }
    size_t values = frames * info.channels;
    if (info.format == FVM_SWF_SOUND_ADPCM)
    { /* Validate the entire declared stream before publishing any caller samples. */
        if (!fvm_sound_adpcm(info, first, frames, 0))
        {
            return FVM_INVALID_ARGUMENT;
        }
        if (frames)
        {
            fvm_sound_adpcm(info, first, frames, destination);
        }
        *written = values;
        return FVM_OK;
    }
    size_t sample_bytes = info.bits_per_sample / FVM_SWF_SOUND_BYTE_BITS;
    const unsigned char *source = info.data + (size_t)first * info.channels * sample_bytes;
    if (info.bits_per_sample == FVM_SWF_SOUND_BITS_8)
    { /* Unsigned offset-binary PCM expands exactly into the high eight S16 bits. */
        for (size_t i = 0; i < values; ++i)
        {
            destination[i] = (short)(((long)source[i] - FVM_SWF_SOUND_U8_BIAS) * FVM_SWF_SOUND_U8_SCALE);
        }
        *written = values;
        return FVM_OK;
    }
    for (size_t i = 0; i < values; ++i)
    {
        unsigned long value;
        if (info.format == FVM_SWF_SOUND_FORMAT_NATIVE)
        {
            unsigned short native;
            std::memcpy(&native, source + i * FVM_SWF_SOUND_PCM_BYTES, sizeof(native));
            value = native;
        }
        else
        {
            value = (unsigned long)source[i * FVM_SWF_SOUND_PCM_BYTES] |
                    ((unsigned long)source[i * FVM_SWF_SOUND_PCM_BYTES + 1] << FVM_SWF_SOUND_BYTE_BITS);
        }
        destination[i] =
            (short)(value >= FVM_SWF_SOUND_SIGN_BIT ? (long)value - FVM_SWF_SOUND_SIGN_RANGE : (long)value);
    }
    *written = values;
    return FVM_OK;
}
#endif
