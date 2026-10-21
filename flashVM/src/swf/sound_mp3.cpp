/* Allocation-free MPEG Layer-III framing and decoding; one scalar static backend for every host. */
#include "sound_mp3.hpp"
#include "sound_constants.h"
#include <climits>
#include <cstring>

/* Parse a complete Layer-III packet without synchronization search or source mutation. */
FVM_Result fvm_swf_mp3_probe(const unsigned char *data, size_t size, FVM_SWFMP3Frame *out)
{
    if (!out || !data || size < FVM_MP3_HEADER_BYTES || data[0] != FVM_MP3_SYNC_FIRST ||
        (data[1] & FVM_MP3_SYNC_MASK) != FVM_MP3_SYNC_MASK)
        return FVM_INVALID_ARGUMENT;
    unsigned int version = (data[1] >> FVM_MP3_VERSION_SHIFT) & FVM_MP3_VERSION_MASK;
    unsigned int layer = (data[1] >> FVM_MP3_LAYER_SHIFT) & FVM_MP3_LAYER_MASK;
    unsigned int bitrate = data[2] >> FVM_MP3_BITRATE_SHIFT;
    unsigned int rate = (data[2] >> FVM_MP3_RATE_SHIFT) & FVM_MP3_RATE_MASK;
    if (version == FVM_MP3_VERSION_RESERVED || !layer || bitrate == FVM_MP3_BITRATE_BAD ||
        rate == FVM_MP3_RATE_RESERVED ||
        (data[3] & FVM_MP3_EMPHASIS_MASK) == FVM_MP3_EMPHASIS_RESERVED)
        return FVM_INVALID_ARGUMENT;
    if (layer != FVM_MP3_LAYER_3 || bitrate == FVM_MP3_BITRATE_FREE)
        return FVM_UNSUPPORTED;
    const unsigned int rates[] = {44100U, 48000U, 32000U};
    const unsigned int bitrates1[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};
    const unsigned int bitrates2[] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160};
    FVM_SWFMP3Frame info;
    info.rate = rates[rate];
    if (version != FVM_MP3_VERSION_1)
        info.rate /= version == FVM_MP3_VERSION_2 ? 2U : 4U;
    info.channels = (data[3] >> FVM_MP3_CHANNEL_SHIFT) == FVM_MP3_MONO_MODE
                        ? FVM_SWF_SOUND_MONO_CHANNELS : FVM_SWF_SOUND_STEREO_CHANNELS;
    info.samples = version == FVM_MP3_VERSION_1 ? FVM_MP3_MPEG1_SAMPLES : FVM_MP3_MPEG2_SAMPLES;
    unsigned long coefficient = version == FVM_MP3_VERSION_1 ? FVM_MP3_MPEG1_COEFFICIENT
                                                              : FVM_MP3_MPEG2_COEFFICIENT;
    info.bytes = coefficient * (version == FVM_MP3_VERSION_1 ? bitrates1[bitrate] : bitrates2[bitrate]) /
                 info.rate + ((data[2] & FVM_MP3_PADDING_MASK) != 0);
    if (info.bytes < FVM_MP3_HEADER_BYTES || info.bytes > size)
        return FVM_INVALID_ARGUMENT;
    *out = info;
    return FVM_OK;
}
/* Initialize reservoir/history without depending on upstream partial-init assumptions. */
FVM_SWFMP3Decoder::FVM_SWFMP3Decoder()
{
    std::memset(&state, 0, sizeof(state));
    fvm_minimp3_init(&state);
}
/* Decode only a prebounded complete packet; refuse concealment after missing reservoir history. */
FVM_Result FVM_SWFMP3Decoder::decode(const unsigned char *data, size_t size, FVM_SWFMP3Frame *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFMP3Frame packet;
    FVM_Result result = fvm_swf_mp3_probe(data, size, &packet);
    if (result != FVM_OK)
        return result;
    if (CHAR_BIT != FVM_SWF_SOUND_BYTE_BITS || sizeof(short) != FVM_SWF_SOUND_PCM_BYTES ||
        SHRT_MAX != FVM_SWF_SOUND_SHORT_MAX || SHRT_MIN != FVM_SWF_SOUND_SHORT_MIN ||
        sizeof(mp3d_sample_t) != sizeof(short) || packet.bytes > INT_MAX)
        return FVM_UNSUPPORTED;
    mp3dec_frame_info_t info;
    std::memset(&info, 0, sizeof(info));
    int frames = fvm_minimp3_decode_frame(&state, data, (int)packet.bytes, pcm, &info);
    if (frames != (int)packet.samples || info.frame_bytes != (int)packet.bytes || info.frame_offset ||
        info.layer != (int)FVM_MP3_DECODED_LAYER_3 || info.hz != (int)packet.rate ||
        info.channels != (int)packet.channels)
        return FVM_INVALID_ARGUMENT;
    *out = packet;
    return FVM_OK;
}
