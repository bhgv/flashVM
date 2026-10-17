/* FlashVM: allocation-free root streaming-sound segments and bounded PCM block extraction.
 * This parser does not execute MovieClip clocks, compressed stream codecs or host output. */
#include "../core/runtime.hpp"
#include "flashvm/swf_stream_sound.h"
#include "reader.hpp"
#include "sound_pcm.hpp"
#include "sound_stream_constants.h"
#include <cstring>

namespace
{
/* Decode the shared nominal header geometry without treating playback advice as conversion. */
void geometry(unsigned int flags, unsigned int &rate, unsigned int &bits, unsigned int &channels)
{
    const unsigned int rates[] = {FVM_SWF_SOUND_RATE_5K, FVM_SWF_SOUND_RATE_11K,
                                  FVM_SWF_SOUND_RATE_22K, FVM_SWF_SOUND_RATE_44K};
    rate = rates[(flags & FVM_SWF_SOUND_RATE_MASK) >> FVM_SWF_SOUND_RATE_SHIFT];
    bits = flags & FVM_SWF_SOUND_SIZE_MASK ? FVM_SWF_SOUND_BITS_16 : FVM_SWF_SOUND_BITS_8;
    channels = flags & FVM_SWF_SOUND_STEREO_MASK ? FVM_SWF_SOUND_STEREO_CHANNELS : FVM_SWF_SOUND_MONO_CHANNELS;
}

/* Validate one exact header; retain opaque codec metadata without claiming compressed decoding. */
FVM_Result head(const unsigned char *data, size_t size, unsigned int tag, unsigned int version,
                FVM_SWFStreamSoundInfo &out)
{
    if (size < FVM_SWF_SOUND_STREAM_HEAD_BYTES ||
        (tag == FVM_SWF_SOUND_STREAM_HEAD2 && version < FVM_SWF_SOUND_STREAM_HEAD2_VERSION))
        return FVM_INVALID_ARGUMENT;
    FVM_SWFReader reader(data, size);
    unsigned int playback = (unsigned int)reader.integer(1);
    unsigned int flags = (unsigned int)reader.integer(1);
    if (playback & FVM_SWF_SOUND_STREAM_RESERVED_MASK)
        return FVM_INVALID_ARGUMENT;
    std::memset(&out, 0, sizeof(out));
    out.struct_size = sizeof(out);
    out.header_tag = tag;
    out.format = flags >> FVM_SWF_SOUND_FORMAT_SHIFT;
    if (size != (out.format == FVM_SWF_SOUND_STREAM_MP3 ? FVM_SWF_SOUND_STREAM_MP3_BYTES : FVM_SWF_SOUND_STREAM_HEAD_BYTES))
        return FVM_INVALID_ARGUMENT;
    geometry(flags, out.sample_rate, out.bits_per_sample, out.channels);
    geometry(playback, out.playback_sample_rate, out.playback_bits_per_sample, out.playback_channels);
    out.average_samples = (unsigned int)reader.integer(FVM_SWF_SOUND_UI16_BYTES);
    if (tag == FVM_SWF_SOUND_STREAM_HEAD &&
        ((out.format != FVM_SWF_SOUND_ADPCM && out.format != FVM_SWF_SOUND_STREAM_MP3) ||
         out.bits_per_sample != FVM_SWF_SOUND_BITS_16 || out.playback_bits_per_sample != FVM_SWF_SOUND_BITS_16))
        return FVM_INVALID_ARGUMENT;
    if (out.format == FVM_SWF_SOUND_PCM_LE && version < FVM_SWF_SOUND_LE_MIN_VERSION)
        return FVM_INVALID_ARGUMENT;
    if (out.format != FVM_SWF_SOUND_PCM_NATIVE && out.format != FVM_SWF_SOUND_PCM_LE &&
        out.bits_per_sample != FVM_SWF_SOUND_BITS_16)
        return FVM_INVALID_ARGUMENT;
    if (out.format == FVM_SWF_SOUND_STREAM_MP3)
    {
        if (version < FVM_SWF_SOUND_STREAM_MP3_VERSION || out.sample_rate == FVM_SWF_SOUND_RATE_5K)
            return FVM_INVALID_ARGUMENT;
        unsigned long latency = reader.integer(FVM_SWF_SOUND_UI16_BYTES);
        out.latency_seek = (int)(latency >= FVM_SWF_SOUND_SIGN_BIT ? (long)latency - FVM_SWF_SOUND_SIGN_RANGE : (long)latency);
    }
    return reader.finished() ? FVM_OK : FVM_INVALID_ARGUMENT;
}

/* Validate the entire root timeline before publishing any segment, block or count. */
FVM_Result scan(FVM_Context *context, size_t wanted, size_t block_ordinal, FVM_SWFStreamSoundInfo *info,
                FVM_SWFStreamSoundBlock *block, size_t *stream_count, size_t *block_count)
{
    /* [GC:] Canonical storage belongs to the context; no managed allocation or callback occurs. */
    if (!context || context->destroying || context->gc_collecting || context->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    if (!context->swf_loaded || CHAR_BIT != FVM_SWF_SOUND_BYTE_BITS)
        return FVM_UNSUPPORTED;
    const fvm_detail::vector<unsigned char> &bytes = context->swf_bytes;
    if (bytes.size() < FVM_SWF_SOUND_MIN_FILE_BYTES)
        return FVM_INVALID_ARGUMENT;
    size_t rectangle = (FVM_SWF_SOUND_RECT_WIDTH_BITS + FVM_SWF_SOUND_RECT_COORDINATES *
        (bytes[FVM_SWF_SOUND_FILE_PREFIX_BYTES] >> FVM_SWF_SOUND_RECT_WIDTH_SHIFT) + FVM_SWF_SOUND_BYTE_ROUNDING) /
        FVM_SWF_SOUND_BYTE_BITS;
    if (rectangle > bytes.size() - FVM_SWF_SOUND_FILE_PREFIX_BYTES ||
        bytes.size() - FVM_SWF_SOUND_FILE_PREFIX_BYTES - rectangle < FVM_SWF_SOUND_MOVIE_HEADER_BYTES)
        return FVM_INVALID_ARGUMENT;
    size_t at = FVM_SWF_SOUND_FILE_PREFIX_BYTES + rectangle + FVM_SWF_SOUND_MOVIE_HEADER_BYTES;
    size_t streams = 0, blocks = 0, selected_count = 0;
    unsigned int frame = 1;
    bool frame_block = false, have_header = false, found_header = false, found_block = false;
    FVM_SWFStreamSoundInfo current, selected;
    FVM_SWFStreamSoundBlock selected_block;
    while (at < bytes.size())
    {
        FVM_SWFReader reader(bytes.begin() + at, bytes.size() - at);
        unsigned int record = (unsigned int)reader.integer(FVM_SWF_SOUND_UI16_BYTES);
        unsigned int tag = record >> FVM_SWF_TAG_CODE_SHIFT;
        unsigned long size = record & FVM_SWF_TAG_LENGTH_MASK;
        if (size == FVM_SWF_TAG_LENGTH_MASK)
            size = reader.integer(FVM_SWF_SOUND_UI32_BYTES);
        if (!reader.good() || size > bytes.size() - at - reader.position())
            return FVM_INVALID_ARGUMENT;
        at += reader.position();
        if (tag == FVM_SWF_SOUND_STREAM_HEAD || tag == FVM_SWF_SOUND_STREAM_HEAD2)
        {
            if (frame > context->swf_info.frame_count)
                return FVM_INVALID_ARGUMENT;
            FVM_Result result = head(bytes.begin() + at, (size_t)size, tag, context->swf_info.version, current);
            if (result != FVM_OK)
                return result;
            current.stream_index = streams++;
            current.frame = frame;
            have_header = true;
            blocks = 0;
            if (current.stream_index == wanted)
            {
                selected = current;
                found_header = true;
            }
        }
        else if (tag == FVM_SWF_SOUND_STREAM_BLOCK)
        {
            if (!have_header || frame_block || frame > context->swf_info.frame_count)
                return FVM_INVALID_ARGUMENT;
            frame_block = true;
            FVM_SWFStreamSoundBlock value;
            std::memset(&value, 0, sizeof(value));
            value.struct_size = sizeof(value);
            value.stream_index = current.stream_index;
            value.block_index = blocks++;
            value.frame = frame;
            value.data = bytes.begin() + at;
            value.data_size = (size_t)size;
            if (current.format == FVM_SWF_SOUND_PCM_NATIVE || current.format == FVM_SWF_SOUND_PCM_LE)
            {
                size_t frame_bytes = current.channels * (current.bits_per_sample / FVM_SWF_SOUND_BYTE_BITS);
                if (value.data_size % frame_bytes)
                    return FVM_INVALID_ARGUMENT;
                value.sample_count_known = 1;
                value.sample_count = value.data_size / frame_bytes;
            }
            if (current.stream_index == wanted)
            {
                ++selected_count;
                if (value.block_index == block_ordinal)
                {
                    selected_block = value;
                    found_block = true;
                }
            }
        }
        else if (tag == FVM_SWF_TAG_SHOW_FRAME)
        {
            if (size || frame > context->swf_info.frame_count)
                return FVM_INVALID_ARGUMENT;
            ++frame;
            frame_block = false;
        }
        at += (size_t)size;
        if (tag == FVM_SWF_TAG_END)
        {
            if (size || at != bytes.size() || frame - 1 != context->swf_info.frame_count)
                return FVM_INVALID_ARGUMENT;
            if ((info || block || block_count) && !found_header)
                return FVM_NOT_FOUND;
            if (block && !found_block)
                return FVM_NOT_FOUND;
            if (info)
                *info = selected;
            if (block)
                *block = selected_block;
            if (stream_count)
                *stream_count = streams;
            if (block_count)
                *block_count = selected_count;
            return FVM_OK;
        }
    }
    return FVM_INVALID_ARGUMENT;
}
} // namespace

/* Count header segments without resolving nested sprite timelines or opening output. */
extern "C" FVM_Result fvm_swf_stream_sound_count(FVM_Context *context, size_t *out)
{
    return out ? scan(context, 0, 0, 0, 0, out, 0) : FVM_INVALID_ARGUMENT;
}
/* Copy one validated stream header, leaving all fields unchanged on failure. */
extern "C" FVM_Result fvm_swf_stream_sound_at(FVM_Context *context, size_t stream, FVM_SWFStreamSoundInfo *out)
{
    return out && out->struct_size == sizeof(*out) ? scan(context, stream, 0, out, 0, 0, 0) : FVM_INVALID_ARGUMENT;
}
/* Count only explicitly encoded blocks governed by this header. */
extern "C" FVM_Result fvm_swf_stream_sound_block_count(FVM_Context *context, size_t stream, size_t *out)
{
    return out ? scan(context, stream, 0, 0, 0, 0, out) : FVM_INVALID_ARGUMENT;
}
/* Publish copied metadata and borrowed payload only after the entire root validates. */
extern "C" FVM_Result fvm_swf_stream_sound_block_at(FVM_Context *context, size_t stream, size_t ordinal,
                                                   FVM_SWFStreamSoundBlock *out)
{
    return out && out->struct_size == sizeof(*out) ? scan(context, stream, ordinal, 0, out, 0, 0) : FVM_INVALID_ARGUMENT;
}
/* Re-resolve a current block before decoding so no retained payload address can become stale. */
extern "C" FVM_Result fvm_swf_stream_sound_read_pcm(FVM_Context *context, size_t stream, size_t ordinal,
                                                    size_t first, size_t frames, short *out, size_t capacity, size_t *written)
{
    if (!written)
        return FVM_INVALID_ARGUMENT;
    *written = 0;
    FVM_SWFStreamSoundInfo header;
    FVM_SWFStreamSoundBlock block;
    FVM_Result result = scan(context, stream, ordinal, &header, &block, 0, 0);
    if (result != FVM_OK)
        return result;
    if (!block.sample_count_known)
        return FVM_UNSUPPORTED;
    if (first > block.sample_count || block.sample_count > ULONG_MAX)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundInfo info;
    std::memset(&info, 0, sizeof(info));
    info.format = header.format;
    info.sample_rate = header.sample_rate;
    info.bits_per_sample = header.bits_per_sample;
    info.channels = header.channels;
    info.sample_count = (unsigned long)block.sample_count;
    info.data = block.data;
    info.data_size = block.data_size;
    return fvm_swf_sound_decode_pcm(info, (unsigned long)first, frames, out, capacity, written);
}
