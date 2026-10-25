/* FlashVM: bounded, allocation-free SWF ADPCM decoding.
 * Packet geometry follows Adobe SWF 19 ADPCMSOUNDDATA: one initial frame
 * followed by up to 4095 interleaved differential frames, without byte alignment.
 * Tables below are the numeric IMA/SWF codec parameters, not program source. */
#ifndef FVM_SWF_SOUND_ADPCM_HPP
#define FVM_SWF_SOUND_ADPCM_HPP /* Guard the private allocation-free decoder. */
#include "sound_constants.h"

namespace
{ /* Bounded MSB-first reader; byte positions avoid overflowing size * CHAR_BIT. */


struct SoundBits
{
    const unsigned char *data; /* Borrowed immutable compressed bytes. */
    size_t size;               /* Available complete bytes. */
    size_t byte;               /* Next source byte. */
    unsigned int bit;          /* Already consumed high bits of the next byte. */


    /* Start at the first compressed bit. */
    SoundBits(const FVM_SWFSoundInfo &info) : data(info.data), size(info.data_size), byte(0), bit(0)
    {
    }


    /* Read a small unsigned field, failing before accessing missing bytes. */
    bool read(unsigned int width, unsigned int &value)
    {
        value = 0;
        for (unsigned int i = 0; i < width; ++i)
        {
            if (byte == size)
            {
                return false;
            }
            value = (value << 1) | ((data[byte] >> (FVM_SWF_SOUND_BYTE_ROUNDING - bit)) & 1U);
            if (++bit == FVM_SWF_SOUND_BYTE_BITS)
            {
                bit = 0;
                ++byte;
            }
        }
        return true;
    }
};


/* Validate/decode all declared frames; optionally publish only the requested range.
 * No VM operations, allocations or callbacks occur between validation and output. */
bool fvm_sound_adpcm(const FVM_SWFSoundInfo &info, unsigned long first, size_t frames, short *output)
{ /* Standard adaptive quantizer step magnitudes indexed by decoder state. */
    static const int steps[] = {
        7,    8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,   28,
        31,   34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,  118,
        130,  143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,  494,
        544,  598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878, 2066,
        2272, 2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845, 8630,
        9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
    /* Per-width magnitude-code changes to the adaptive step index. */
    static const int adjustments[][16] = {{-1, 2},
                                          {-1, -1, 2, 4},
                                          {-1, -1, -1, -1, 2, 4, 6, 8},
                                          {-1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 6, 8, 10, 13, 16}};


    SoundBits bits(info);
    unsigned int encoded_width;
    if (!bits.read(FVM_SWF_ADPCM_WIDTH_BITS, encoded_width))
    {
        return false;
    }
    unsigned int width = encoded_width + FVM_SWF_ADPCM_MIN_WIDTH;
    unsigned int sign = 1U << (width - 1);
    long predictor[FVM_SWF_SOUND_STEREO_CHANNELS] = {0, 0};
    int index[FVM_SWF_SOUND_STEREO_CHANNELS] = {0, 0};
    size_t written = 0;
    for (unsigned long frame = 0; frame < info.sample_count; ++frame)
    {
        for (unsigned int channel = 0; channel < info.channels; ++channel)
        {
            unsigned int code;
            if (frame % FVM_SWF_ADPCM_BLOCK_FRAMES == 0)
            {
                if (!bits.read(FVM_SWF_SOUND_BITS_16, code))
                {
                    return false;
                }
                predictor[channel] =
                    code >= FVM_SWF_SOUND_SIGN_BIT ? (long)code - FVM_SWF_SOUND_SIGN_RANGE : (long)code;
                if (!bits.read(FVM_SWF_ADPCM_INDEX_BITS, code))
                {
                    return false;
                }
                index[channel] = (int)code;
            }
            else
            {
                if (!bits.read(width, code))
                {
                    return false;
                }
                unsigned int magnitude = code & (sign - 1);
                long step = steps[index[channel]];
                long difference = step >> (width - 1);
                for (unsigned int shift = 0; shift < width - 1; ++shift)
                {
                    if (magnitude & (sign >> (shift + 1)))
                    {
                        difference += step >> shift;
                    }
                }
                predictor[channel] += code & sign ? -difference : difference;
                if (predictor[channel] < FVM_SWF_ADPCM_SAMPLE_MIN)
                {
                    predictor[channel] = FVM_SWF_ADPCM_SAMPLE_MIN;
                }
                if (predictor[channel] > FVM_SWF_SOUND_SHORT_MAX)
                {
                    predictor[channel] = FVM_SWF_SOUND_SHORT_MAX;
                }
                index[channel] += adjustments[encoded_width][magnitude];
                if (index[channel] < 0)
                {
                    index[channel] = 0;
                }
                if (index[channel] > FVM_SWF_ADPCM_INDEX_MAX)
                {
                    index[channel] = FVM_SWF_ADPCM_INDEX_MAX;
                }
            }
            if (output && frame >= first && frame - first < frames)
            {
                output[written++] = (short)predictor[channel];
            }
        }
    }
    return true;
}
} // namespace
#endif
