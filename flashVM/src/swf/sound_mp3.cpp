/* Allocation-free MPEG Layer-III framing and decoding; one scalar static backend for every host. */
#include "sound_mp3.hpp"
#include "sound_constants.h"
#include <climits>
#include <cstring>

/* Parse a Layer-III header; zero bytes denotes a valid free-format header awaiting geometry. */
static FVM_Result mp3_header(const unsigned char *data, size_t size, FVM_SWFMP3Frame *out)
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
    if (layer != FVM_MP3_LAYER_3)
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
    if (bitrate == FVM_MP3_BITRATE_FREE)
        info.bytes = 0;
    else if (info.bytes < FVM_MP3_HEADER_BYTES || info.bytes > size)
        return FVM_INVALID_ARGUMENT;
    *out = info;
    return FVM_OK;
}
/* Require exact free-format geometry, without accepting a skipped byte or a different source clock. */
static bool free_header(const unsigned char *data, size_t size, const FVM_SWFMP3Frame &first)
{
    FVM_SWFMP3Frame next;
    return mp3_header(data, size, &next) == FVM_OK && !next.bytes &&
           next.rate == first.rate && next.channels == first.channels && next.samples == first.samples;
}
/* Infer an unpadded free-format size from three consecutive matching headers at offset zero. */
FVM_Result fvm_swf_mp3_probe(const unsigned char *data, size_t size, FVM_SWFMP3Frame *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFMP3Frame info;
    FVM_Result result = mp3_header(data, size, &info);
    if (result != FVM_OK)
        return result;
    if (!info.bytes)
    {
        size_t padding = (data[2] & FVM_MP3_PADDING_MASK) != 0;
        size_t chosen = 0;
        for (size_t span = FVM_MP3_HEADER_BYTES; span < FVM_MP3_FREE_MAX_BYTES; ++span)
        {
            if (span > size || size - span < FVM_MP3_HEADER_BYTES)
                break;
            if (!free_header(data + span, size - span, info))
                continue;
            size_t base = span - padding;
            size_t second = base + ((data[span + 2] & FVM_MP3_PADDING_MASK) != 0);
            if (second > size - span || size - span - second < FVM_MP3_HEADER_BYTES ||
                !free_header(data + span + second, size - span - second, info))
                continue;
            chosen = span;
            break; /* Select the smallest verified span, not a multiple spanning whole packets. */
        }
        if (!chosen)
            return FVM_UNSUPPORTED; /* Too little synchronization evidence for an event packet. */
        info.bytes = chosen;
    }
    *out = info;
    return FVM_OK;
}
/* Initialize reservoir/history without depending on upstream partial-init assumptions. */
FVM_SWFMP3Decoder::FVM_SWFMP3Decoder()
{
    std::memset(&state, 0, sizeof(state));
    fvm_minimp3_init(&state);
}
/* Infer one block's unpadded free-format span using its author-provided exact packet count. */
FVM_Result FVM_SWFMP3Decoder::prepare_block(const unsigned char *data, size_t size,
                                            unsigned int samples)
{
    if (!size)
        return samples ? FVM_INVALID_ARGUMENT : FVM_OK;
    FVM_SWFMP3Frame first;
    FVM_Result result = mp3_header(data, size, &first);
    if (result != FVM_OK || first.bytes || state.free_format_bytes)
        return result;
    if (state.header[0])
        return FVM_UNSUPPORTED; /* A different bitrate mode cannot reuse an established reservoir. */
    if (!samples || samples > FVM_MP3_BLOCK_MAX_SAMPLES || samples % first.samples)
        return FVM_INVALID_ARGUMENT;
    size_t packets = samples / first.samples;
    size_t chosen = 0;
    for (size_t base = FVM_MP3_HEADER_BYTES; base < FVM_MP3_FREE_MAX_BYTES; ++base)
    {
        size_t at = 0;
        bool valid = true;
        for (size_t index = 0; index < packets; ++index)
        {
            if (at > size || !free_header(data + at, size - at, first))
            {
                valid = false;
                break;
            }
            size_t span = base + ((data[at + 2] & FVM_MP3_PADDING_MASK) != 0);
            if (span >= FVM_MP3_FREE_MAX_BYTES || span > size - at)
            {
                valid = false;
                break;
            }
            at += span;
        }
        if (valid && at == size)
        {
            if (chosen)
                return FVM_INVALID_ARGUMENT;
            chosen = base;
        }
    }
    if (!chosen)
        return FVM_INVALID_ARGUMENT;
    /* Commit only geometry; reservoir/filter history remain pristine until the first decode. */
    std::memcpy(state.header, data, FVM_MP3_HEADER_BYTES);
    state.free_format_bytes = (int)chosen;
    return FVM_OK;
}
/* Decode only a prebounded complete packet; refuse concealment after missing reservoir history. */
FVM_Result FVM_SWFMP3Decoder::decode(const unsigned char *data, size_t size, FVM_SWFMP3Frame *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFMP3Frame packet;
    FVM_Result result = mp3_header(data, size, &packet);
    if (result != FVM_OK)
        return result;
    bool free_format = !packet.bytes;
    if (free_format)
    {
        if (state.free_format_bytes)
        {
            packet.bytes = (size_t)state.free_format_bytes + ((data[2] & FVM_MP3_PADDING_MASK) != 0);
            if (packet.bytes > size)
                return FVM_INVALID_ARGUMENT;
        }
        else
        {
            result = fvm_swf_mp3_probe(data, size, &packet);
            if (result != FVM_OK)
                return result;
        }
    }
    if (CHAR_BIT != FVM_SWF_SOUND_BYTE_BITS || sizeof(short) != FVM_SWF_SOUND_PCM_BYTES ||
        SHRT_MAX != FVM_SWF_SOUND_SHORT_MAX || SHRT_MIN != FVM_SWF_SOUND_SHORT_MIN ||
        sizeof(mp3d_sample_t) != sizeof(short) || packet.bytes > INT_MAX)
        return FVM_UNSUPPORTED;
    if (free_format && !state.free_format_bytes)
    {
        /* Seed only verified geometry into the pinned decoder; reservoir/history remain zero. */
        if (state.header[0])
            return FVM_UNSUPPORTED;
        std::memcpy(state.header, data, FVM_MP3_HEADER_BYTES);
        state.free_format_bytes = (int)(packet.bytes - ((data[2] & FVM_MP3_PADDING_MASK) != 0));
    }
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

/* Decode one complete event pass, optionally copying only the selected declared source interval. */
static FVM_Result event_pass(const FVM_SWFSoundInfo &source, size_t skip,
                              unsigned long first, size_t frames, short *out)
{
    FVM_SWFMP3Decoder decoder;
    size_t at = FVM_MP3_SEEK_BYTES, total = 0;
    if (source.sample_count > (size_t)-1 - skip)
        return FVM_OUT_OF_MEMORY;
    size_t required = skip + (size_t)source.sample_count;
    size_t begin = skip + (size_t)first;
    size_t end = begin + frames;
    while (at < source.data_size)
    {
        FVM_SWFMP3Frame packet;
        FVM_Result result = decoder.decode(source.data + at, source.data_size - at, &packet);
        if (result != FVM_OK)
            return result;
        if (packet.rate != source.sample_rate || packet.channels != source.channels)
            return FVM_INVALID_ARGUMENT;
        if (packet.samples > (size_t)-1 - total)
            return FVM_OUT_OF_MEMORY;
        size_t next = total + packet.samples;
        if (out && begin < next && end > total)
        {
            size_t left = begin > total ? begin : total;
            size_t right = end < next ? end : next;
            std::memcpy(out + (left - begin) * packet.channels,
                        decoder.pcm + (left - total) * packet.channels,
                        (right - left) * packet.channels * sizeof(short));
        }
        at += packet.bytes;
        total = next;
    }
    return total >= required ? FVM_OK : FVM_INVALID_ARGUMENT;
}
/* Validate complete source first so malformed suffixes cannot partially overwrite a range. */
FVM_Result fvm_swf_mp3_read(const FVM_SWFSoundInfo &source, unsigned long first, size_t frames,
                            short *out, size_t capacity, size_t *written)
{
    if (!written)
        return FVM_INVALID_ARGUMENT;
    *written = 0;
    if (source.format != FVM_SWF_SOUND_MP3)
        return FVM_UNSUPPORTED;
    if (!source.data || source.data_size < FVM_MP3_SEEK_BYTES ||
        source.channels < FVM_SWF_SOUND_MONO_CHANNELS || source.channels > FVM_SWF_SOUND_STEREO_CHANNELS ||
        first > source.sample_count || frames > source.sample_count - first ||
        frames > capacity / source.channels || (frames && !out))
        return FVM_INVALID_ARGUMENT;
    unsigned long raw = (unsigned long)source.data[0] |
                        ((unsigned long)source.data[1] << FVM_SWF_SOUND_BYTE_BITS);
    if (raw >= FVM_SWF_SOUND_SIGN_BIT)
        return FVM_UNSUPPORTED;
    FVM_Result result = event_pass(source, (size_t)raw, first, frames, 0);
    if (result != FVM_OK)
        return result;
    if (frames)
    {
        result = event_pass(source, (size_t)raw, first, frames, out);
        if (result != FVM_OK)
            return result;
    }
    *written = frames * source.channels;
    return FVM_OK;
}
