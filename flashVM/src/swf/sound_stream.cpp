/* FlashVM: live root streaming-sound inspection and optional owned indexed encoded snapshots.
 * Explicit preparation expands canonical whole, uniquely inferred or counted partial ADPCM; no MovieClip clock or host output runs. */
#include "../core/runtime.hpp"
#include "flashvm/swf_stream_sound.h"
#include "reader.hpp"
#include "sound_pcm.hpp"
#include "sound_stream_constants.h"
#include <cstring>
#include <new>

#include "sound_stream_internal.hpp"

namespace
{
/* Decode the shared nominal header geometry without treating playback advice as conversion. */
void geometry(unsigned int flags, unsigned int &rate, unsigned int &bits, unsigned int &channels)
{
    const unsigned int rates[] = {FVM_SWF_SOUND_RATE_5K, FVM_SWF_SOUND_RATE_11K,
                                  FVM_SWF_SOUND_RATE_22K, FVM_SWF_SOUND_RATE_44K};
    rate = rates[(flags & FVM_SWF_SOUND_RATE_MASK) >> FVM_SWF_SOUND_RATE_SHIFT];
    bits = flags & FVM_SWF_SOUND_SIZE_MASK ? FVM_SWF_SOUND_BITS_16 : FVM_SWF_SOUND_BITS_8;
    channels = flags & FVM_SWF_SOUND_STEREO_MASK ? FVM_SWF_SOUND_STEREO_CHANNELS
                                                 : FVM_SWF_SOUND_MONO_CHANNELS;
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
    if (size != (out.format == FVM_SWF_SOUND_STREAM_MP3 ? FVM_SWF_SOUND_STREAM_MP3_BYTES
                                                        : FVM_SWF_SOUND_STREAM_HEAD_BYTES))
        return FVM_INVALID_ARGUMENT;
    geometry(flags, out.sample_rate, out.bits_per_sample, out.channels);
    geometry(playback, out.playback_sample_rate, out.playback_bits_per_sample,
             out.playback_channels);
    out.average_samples = (unsigned int)reader.integer(FVM_SWF_SOUND_UI16_BYTES);
    if (tag == FVM_SWF_SOUND_STREAM_HEAD &&
        ((out.format != FVM_SWF_SOUND_ADPCM && out.format != FVM_SWF_SOUND_STREAM_MP3) ||
         out.bits_per_sample != FVM_SWF_SOUND_BITS_16 ||
         out.playback_bits_per_sample != FVM_SWF_SOUND_BITS_16))
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
        out.latency_seek =
            (int)(latency >= FVM_SWF_SOUND_SIGN_BIT ? (long)latency - FVM_SWF_SOUND_SIGN_RANGE
                                                    : (long)latency);
    }
    return reader.finished() ? FVM_OK : FVM_INVALID_ARGUMENT;
}

/* Validate the entire root timeline before publishing any segment, block or count. */
FVM_Result scan(FVM_Context *context, size_t wanted, size_t block_ordinal,
                FVM_SWFStreamSoundInfo *info, FVM_SWFStreamSoundBlock *block, size_t *stream_count,
                size_t *block_count, size_t *payload_size = 0,
                FVM_SWFStreamSoundSnapshot *capture = 0)
{
    /* [GC:] Canonical storage belongs to the context; no managed allocation or callback occurs. */
    if (!context || context->destroying || context->gc_collecting || context->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    if (!context->swf_loaded || CHAR_BIT != FVM_SWF_SOUND_BYTE_BITS)
        return FVM_UNSUPPORTED;
    const fvm_detail::vector<unsigned char> &bytes = context->swf_bytes;
    if (bytes.size() < FVM_SWF_SOUND_MIN_FILE_BYTES)
        return FVM_INVALID_ARGUMENT;
    size_t rectangle = (FVM_SWF_SOUND_RECT_WIDTH_BITS +
                        FVM_SWF_SOUND_RECT_COORDINATES * (bytes[FVM_SWF_SOUND_FILE_PREFIX_BYTES] >>
                                                          FVM_SWF_SOUND_RECT_WIDTH_SHIFT) +
                        FVM_SWF_SOUND_BYTE_ROUNDING) /
                       FVM_SWF_SOUND_BYTE_BITS;
    if (rectangle > bytes.size() - FVM_SWF_SOUND_FILE_PREFIX_BYTES ||
        bytes.size() - FVM_SWF_SOUND_FILE_PREFIX_BYTES - rectangle <
            FVM_SWF_SOUND_MOVIE_HEADER_BYTES)
        return FVM_INVALID_ARGUMENT;
    size_t at = FVM_SWF_SOUND_FILE_PREFIX_BYTES + rectangle + FVM_SWF_SOUND_MOVIE_HEADER_BYTES;
    size_t streams = 0, blocks = 0, selected_count = 0, payload = 0;
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
            FVM_Result result =
                head(bytes.begin() + at, (size_t)size, tag, context->swf_info.version, current);
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
            if (current.format == FVM_SWF_SOUND_PCM_NATIVE ||
                current.format == FVM_SWF_SOUND_PCM_LE)
            {
                size_t frame_bytes =
                    current.channels * (current.bits_per_sample / FVM_SWF_SOUND_BYTE_BITS);
                if (value.data_size % frame_bytes)
                    return FVM_INVALID_ARGUMENT;
                value.sample_count_known = 1;
                value.sample_count = value.data_size / frame_bytes;
            }
            if (current.stream_index == wanted)
            {
                if (value.data_size > (size_t)-1 - payload)
                    return FVM_OUT_OF_MEMORY;
                if (capture)
                {
                    FVM_SWFStreamSoundEntry &entry = capture->blocks[selected_count];
                    entry.block = value;
                    entry.block.data = 0;
                    entry.offset = payload;
                    if (value.data_size)
                        std::memcpy(capture->bytes.begin() + payload, value.data, value.data_size);
                }
                payload += value.data_size;
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
            if (payload_size)
                *payload_size = payload;
            return FVM_OK;
        }
    }
    return FVM_INVALID_ARGUMENT;
}
/* Parse only the fixed SWF prefix, without certifying the enclosed MPEG frames.
 * [GC:] Both spans are native immutable storage; no callback or allocation can invalidate them. */
FVM_Result mp3_block(const FVM_SWFStreamSoundInfo &info, const FVM_SWFStreamSoundBlock &block,
                     FVM_SWFStreamSoundMP3Block *out)
{
    if (info.format != FVM_SWF_SOUND_STREAM_MP3)
        return FVM_UNSUPPORTED;
    if (block.data_size < FVM_SWF_SOUND_STREAM_MP3_PREFIX_BYTES)
        return FVM_INVALID_ARGUMENT;

    /* Copy into a local record so malformed input cannot partially publish output. */
    FVM_SWFReader reader(block.data, block.data_size);
    FVM_SWFStreamSoundMP3Block value;
    std::memset(&value, 0, sizeof(value));
    value.struct_size = sizeof(value);
    value.stream_index = block.stream_index;
    value.block_index = block.block_index;
    value.frame = block.frame;
    value.declared_samples = (unsigned int)reader.integer(FVM_SWF_SOUND_UI16_BYTES);
    unsigned long seek = reader.integer(FVM_SWF_SOUND_UI16_BYTES);
    value.seek_samples =
        (int)(seek >= FVM_SWF_SOUND_SIGN_BIT ? (long)seek - FVM_SWF_SOUND_SIGN_RANGE : (long)seek);
    value.data = block.data + FVM_SWF_SOUND_STREAM_MP3_PREFIX_BYTES;
    value.data_size = block.data_size - FVM_SWF_SOUND_STREAM_MP3_PREFIX_BYTES;
    *out = value;
    return FVM_OK;
}
} // namespace

/* Count header segments without resolving nested sprite timelines or opening output. */
extern "C" FVM_Result fvm_swf_stream_sound_count(FVM_Context *context, size_t *out)
{
    return out ? scan(context, 0, 0, 0, 0, out, 0) : FVM_INVALID_ARGUMENT;
}
/* Copy one validated stream header, leaving all fields unchanged on failure. */
extern "C" FVM_Result fvm_swf_stream_sound_at(FVM_Context *context, size_t stream,
                                              FVM_SWFStreamSoundInfo *out)
{
    return out && out->struct_size == sizeof(*out) ? scan(context, stream, 0, out, 0, 0, 0)
                                                   : FVM_INVALID_ARGUMENT;
}
/* Count only explicitly encoded blocks governed by this header. */
extern "C" FVM_Result fvm_swf_stream_sound_block_count(FVM_Context *context, size_t stream,
                                                       size_t *out)
{
    return out ? scan(context, stream, 0, 0, 0, 0, out) : FVM_INVALID_ARGUMENT;
}
/* Publish copied metadata and borrowed payload only after the entire root validates. */
extern "C" FVM_Result fvm_swf_stream_sound_block_at(FVM_Context *context, size_t stream,
                                                    size_t ordinal, FVM_SWFStreamSoundBlock *out)
{
    return out && out->struct_size == sizeof(*out) ? scan(context, stream, ordinal, 0, out, 0, 0)
                                                   : FVM_INVALID_ARGUMENT;
}
/* Resolve the current movie and selected block before publishing a borrowed MPEG span. */
extern "C" FVM_Result fvm_swf_stream_sound_mp3_block_at(FVM_Context *context, size_t stream,
                                                        size_t ordinal,
                                                        FVM_SWFStreamSoundMP3Block *out)
{
    if (!out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    FVM_SWFStreamSoundInfo info;
    FVM_SWFStreamSoundBlock block;
    FVM_Result result = scan(context, stream, ordinal, &info, &block, 0, 0);
    return result == FVM_OK ? mp3_block(info, block, out) : result;
}
/* Re-resolve a current block before decoding so no retained payload address can become stale. */
extern "C" FVM_Result fvm_swf_stream_sound_read_pcm(FVM_Context *context, size_t stream,
                                                    size_t ordinal, size_t first, size_t frames,
                                                    short *out, size_t capacity, size_t *written)
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

/* Prepare an indexed immutable encoded segment in two linear scans, publishing only on success. */
static FVM_Result snapshot_open(FVM_Context *context, size_t stream, const size_t *budget,
                                FVM_SWFStreamSoundSnapshot **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    FVM_SWFStreamSoundInfo info;
    size_t count = 0, bytes = 0;
    FVM_Result result = scan(context, stream, 0, &info, 0, 0, &count, &bytes);
    if (result != FVM_OK)
        return result;
    size_t limit = context->runtime->config.memory_limit_bytes
                       ? context->runtime->config.memory_limit_bytes
                       : FVM_SWF_DEFAULT_LIMIT;
    if (bytes > limit || count > (limit - bytes) / sizeof(FVM_SWFStreamSoundEntry))
        return FVM_OUT_OF_MEMORY;
    size_t charge = bytes + count * sizeof(FVM_SWFStreamSoundEntry);
    if (charge > (size_t)-1 - sizeof(FVM_SWFStreamSoundSnapshot))
        return FVM_OUT_OF_MEMORY;
    charge += sizeof(FVM_SWFStreamSoundSnapshot);
    if (budget && charge > *budget)
        return FVM_OUT_OF_MEMORY;
    FVM_SWFStreamSoundSnapshot *snapshot = 0;
    try
    {
        snapshot = new FVM_SWFStreamSoundSnapshot;
        snapshot->empty = 0;
        snapshot->prepared_mp3 = false;
        snapshot->retained_bytes = snapshot->peak_bytes = charge;
        snapshot->info = info;
        snapshot->blocks.resize(count);
        snapshot->bytes.resize(bytes);
        result = scan(context, stream, 0, 0, 0, 0, 0, 0, snapshot);
        if (result != FVM_OK)
        {
            delete snapshot;
            return result;
        }
        *out = snapshot;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        delete snapshot;
        return FVM_OUT_OF_MEMORY;
    }
}
/* Preserve the original per-payload runtime bound and original encoded representation. */
extern "C" FVM_Result fvm_swf_stream_sound_snapshot_open(FVM_Context *context, size_t stream,
                                                         FVM_SWFStreamSoundSnapshot **out)
{
    return snapshot_open(context, stream, 0, out);
}
/* [GC:] Bound independent encoded ownership before any native snapshot allocation. */
extern "C" FVM_Result
fvm_swf_stream_sound_snapshot_open_bounded(FVM_Context *context, size_t stream, size_t budget,
                                          FVM_SWFStreamSoundSnapshot **out)
{
    return snapshot_open(context, stream, &budget, out);
}
/* Infer only whole 4096-frame ADPCM packets; an average header is never a sample count.
 * Partial packets have no explicit count in this format and remain deliberately unsupported. */
static FVM_Result stream_adpcm_frames(const unsigned char *data, size_t size, unsigned int channels,
                                      size_t *frames)
{
    if (!size)
    {
        *frames = 0;
        return FVM_OK;
    }
    if (size > (size_t)-1 / FVM_SWF_SOUND_BYTE_BITS)
        return FVM_OUT_OF_MEMORY;
    unsigned int width = (data[0] >> FVM_SWF_STREAM_ADPCM_WIDTH_SHIFT) + FVM_SWF_ADPCM_MIN_WIDTH;
    size_t packet =
        channels * (FVM_SWF_STREAM_ADPCM_INITIAL_BITS + (FVM_SWF_ADPCM_BLOCK_FRAMES - 1) * width);
    size_t bits = size * FVM_SWF_SOUND_BYTE_BITS - FVM_SWF_ADPCM_WIDTH_BITS;
    size_t count = bits / packet, padding = bits % packet;
    if (padding > FVM_SWF_STREAM_ADPCM_PADDING_BITS)
        return FVM_UNSUPPORTED;
    if (padding && (data[size - 1] & ((1U << padding) - 1U)))
        return FVM_INVALID_ARGUMENT;
    if (count > ULONG_MAX / FVM_SWF_ADPCM_BLOCK_FRAMES ||
        count > (size_t)-1 / FVM_SWF_ADPCM_BLOCK_FRAMES)
        return FVM_OUT_OF_MEMORY;
    *frames = count * FVM_SWF_ADPCM_BLOCK_FRAMES;
    return FVM_OK;
}
/* Preserve canonical whole-packet preparation, otherwise accept exactly one zero-padded length.
 * At most eight bit-end positions are inspected; averages and movie clocks are never consulted. */
static FVM_Result stream_adpcm_inferred_frames(const unsigned char *data, size_t size,
                                                unsigned int channels, size_t *frames)
{
    FVM_Result legacy = stream_adpcm_frames(data, size, channels, frames);
    if (legacy != FVM_UNSUPPORTED) return legacy;
    /* The legacy check already proved nonempty size*BYTE_BITS is representable. */
    unsigned int width = (data[0] >> FVM_SWF_STREAM_ADPCM_WIDTH_SHIFT) + FVM_SWF_ADPCM_MIN_WIDTH;
    size_t initial = channels * FVM_SWF_STREAM_ADPCM_INITIAL_BITS;
    size_t step = channels * width;
    size_t packet = initial + (FVM_SWF_ADPCM_BLOCK_FRAMES - 1) * step;
    size_t bits = size * FVM_SWF_SOUND_BYTE_BITS - FVM_SWF_ADPCM_WIDTH_BITS;
    size_t selected = 0;
    unsigned int candidates = 0;
    for (unsigned int padding = 0; padding <= FVM_SWF_STREAM_ADPCM_PADDING_BITS; ++padding)
    {
        if (padding > bits || (padding && (data[size - 1] & ((1U << padding) - 1U)))) continue;
        size_t payload = bits - padding;
        size_t whole = payload / packet, remainder = payload % packet;
        size_t tail = 0;
        if (remainder)
        {
            if (remainder < initial || (remainder - initial) % step) continue;
            tail = 1 + (remainder - initial) / step;
        }
        if (whole > ((size_t)-1 - tail) / FVM_SWF_ADPCM_BLOCK_FRAMES ||
            whole > (ULONG_MAX - tail) / FVM_SWF_ADPCM_BLOCK_FRAMES) return FVM_OUT_OF_MEMORY;
        selected = whole * FVM_SWF_ADPCM_BLOCK_FRAMES + tail;
        if (++candidates > 1) return FVM_UNSUPPORTED;
    }
    if (!candidates) return FVM_INVALID_ARGUMENT;
    *frames = selected;
    return FVM_OK;
}
/* Validate an externally supplied exact frame count without guessing padding as sample data.
 * ADPCM packets share bit alignment; only the final block byte may contain zero padding. */
static FVM_Result stream_adpcm_counted(const unsigned char *data, size_t size,
                                       unsigned int channels, size_t frames)
{
    if (frames > ULONG_MAX)
        return FVM_INVALID_ARGUMENT;
    if (!size)
        return frames ? FVM_INVALID_ARGUMENT : FVM_OK;
    unsigned int width = (data[0] >> FVM_SWF_STREAM_ADPCM_WIDTH_SHIFT) + FVM_SWF_ADPCM_MIN_WIDTH;
    size_t packet =
        channels * (FVM_SWF_STREAM_ADPCM_INITIAL_BITS + (FVM_SWF_ADPCM_BLOCK_FRAMES - 1) * width);
    size_t whole = frames / FVM_SWF_ADPCM_BLOCK_FRAMES;
    size_t tail = frames % FVM_SWF_ADPCM_BLOCK_FRAMES;
    size_t extra = tail ? channels * (FVM_SWF_STREAM_ADPCM_INITIAL_BITS + (tail - 1) * width) : 0;
    if (whole > ((size_t)-1 - FVM_SWF_ADPCM_WIDTH_BITS - extra) / packet)
        return FVM_OUT_OF_MEMORY;
    size_t bits = FVM_SWF_ADPCM_WIDTH_BITS + whole * packet + extra;
    size_t bytes = bits / FVM_SWF_SOUND_BYTE_BITS + (bits % FVM_SWF_SOUND_BYTE_BITS != 0);
    if (bytes != size)
        return FVM_INVALID_ARGUMENT;
    unsigned int padding = (unsigned int)((FVM_SWF_SOUND_BYTE_BITS -
                                          bits % FVM_SWF_SOUND_BYTE_BITS) % FVM_SWF_SOUND_BYTE_BITS);
    if (padding && (data[size - 1] & ((1U << padding) - 1U)))
        return FVM_INVALID_ARGUMENT;
    return FVM_OK;
}
/* Validate the complete authoritative block before publishing any selected PCM range.
 * All temporary state is on the stack; neither a live VM nor native owner is mutated. */
static FVM_Result read_adpcm_block(const FVM_SWFStreamSoundInfo &header,
                                    const FVM_SWFStreamSoundBlock &block, size_t samples,
                                    size_t first, size_t frames, short *out, size_t capacity,
                                    size_t *written)
{
    if (header.format != FVM_SWF_SOUND_ADPCM || sizeof(short) != FVM_SWF_SOUND_PCM_BYTES ||
        SHRT_MAX != FVM_SWF_SOUND_SHORT_MAX || SHRT_MIN != FVM_SWF_SOUND_SHORT_MIN)
        return FVM_UNSUPPORTED;
    if (first > samples || frames > samples - first)
        return FVM_INVALID_ARGUMENT;
    FVM_Result result = stream_adpcm_counted(block.data, block.data_size, header.channels, samples);
    if (result != FVM_OK)
        return result;
    FVM_SWFSoundInfo info;
    std::memset(&info, 0, sizeof(info));
    info.format = FVM_SWF_SOUND_ADPCM;
    info.sample_rate = header.sample_rate;
    info.bits_per_sample = FVM_SWF_SOUND_BITS_16;
    info.channels = header.channels;
    info.sample_count = (unsigned long)samples;
    info.data = block.data;
    info.data_size = block.data_size;
    /* Empty payloads have no width field for the shared DefineSound decoder. */
    return !samples ? FVM_OK : fvm_swf_sound_decode_pcm(info, (unsigned long)first, frames,
                                                       out, capacity, written);
}
/* [GC:] Owner-only source scan and stack decoder; no script, allocation or source mutation. */
extern "C" FVM_Result
fvm_swf_stream_sound_read_adpcm(FVM_Context *context, size_t stream, size_t ordinal,
                                 size_t samples, size_t first, size_t frames,
                                 short *out, size_t capacity, size_t *written)
{
    if (!written)
        return FVM_INVALID_ARGUMENT;
    *written = 0;
    FVM_SWFStreamSoundInfo header;
    FVM_SWFStreamSoundBlock block;
    FVM_Result result = scan(context, stream, ordinal, &header, &block, 0, 0);
    return result == FVM_OK ? read_adpcm_block(header, block, samples, first, frames,
                                                out, capacity, written) : result;
}
/* Decode borrowed immutable encoded storage after source teardown with no new native ownership. */
extern "C" FVM_Result
fvm_swf_stream_sound_snapshot_read_adpcm(const FVM_SWFStreamSoundSnapshot *snapshot,
                                          size_t ordinal, size_t samples, size_t first,
                                          size_t frames, short *out, size_t capacity, size_t *written)
{
    if (!written)
        return FVM_INVALID_ARGUMENT;
    *written = 0;
    if (!snapshot)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFStreamSoundBlock block;
    block.struct_size = sizeof(block);
    FVM_Result result = fvm_swf_stream_sound_snapshot_block_at(snapshot, ordinal, &block);
    return result == FVM_OK ? read_adpcm_block(snapshot->info, block, samples, first, frames,
                                                out, capacity, written) : result;
}
/* [GC:] Prepare independent native PCM before worker publication; the temporary never escapes.
 * Encoded plus decoded payload and block-index bytes share the runtime preparation budget. */
static FVM_Result snapshot_open_pcm(FVM_Context *context, size_t stream, const size_t *budget,
                                    const size_t *counts, const size_t *count, bool inferred,
                                    FVM_SWFStreamSoundSnapshot **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (count && *count && !counts)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFStreamSoundSnapshot *snapshot = 0;
    FVM_Result result = snapshot_open(context, stream, budget, &snapshot);
    if (result != FVM_OK)
        return result;
    if (count && (*count != snapshot->blocks.size() ||
                  snapshot->info.format != FVM_SWF_SOUND_ADPCM))
    {
        FVM_Result failure = *count != snapshot->blocks.size() ? FVM_INVALID_ARGUMENT
                                                               : FVM_UNSUPPORTED;
        delete snapshot;
        return failure;
    }
    if (snapshot->info.format == FVM_SWF_SOUND_PCM_NATIVE ||
        snapshot->info.format == FVM_SWF_SOUND_PCM_LE)
    {
        *out = snapshot;
        return FVM_OK;
    }
    if (snapshot->info.format == FVM_SWF_SOUND_STREAM_MP3)
    {
        size_t limit = context->runtime->config.memory_limit_bytes
                           ? context->runtime->config.memory_limit_bytes : FVM_SWF_DEFAULT_LIMIT;
        return fvm_swf_stream_prepare_mp3(snapshot, limit, budget, out);
    }
    if (snapshot->info.format != FVM_SWF_SOUND_ADPCM || sizeof(short) != FVM_SWF_SOUND_PCM_BYTES ||
        SHRT_MAX != FVM_SWF_SOUND_SHORT_MAX || SHRT_MIN != FVM_SWF_SOUND_SHORT_MIN)
    {
        delete snapshot;
        return FVM_UNSUPPORTED;
    }
    size_t limit = context->runtime->config.memory_limit_bytes
                       ? context->runtime->config.memory_limit_bytes
                       : FVM_SWF_DEFAULT_LIMIT;
    size_t used =
        snapshot->bytes.size() + snapshot->blocks.size() * sizeof(FVM_SWFStreamSoundEntry);
    size_t values = 0;
    for (size_t i = 0; i < snapshot->blocks.size(); ++i)
    {
        FVM_SWFStreamSoundEntry &entry = snapshot->blocks[i];
        const unsigned char *data =
            snapshot->bytes.empty() ? &snapshot->empty : snapshot->bytes.begin() + entry.offset;
        size_t frames = count ? counts[i] : 0;
        result = count ? stream_adpcm_counted(data, entry.block.data_size,
                                               snapshot->info.channels, frames)
                       : (inferred ? stream_adpcm_inferred_frames : stream_adpcm_frames)(
                             data, entry.block.data_size, snapshot->info.channels, &frames);
        if (result != FVM_OK ||
            frames > (limit - used) / FVM_SWF_SOUND_PCM_BYTES / snapshot->info.channels)
        {
            delete snapshot;
            return result == FVM_OK ? FVM_OUT_OF_MEMORY : result;
        }
        size_t block_values = frames * snapshot->info.channels;
        size_t decoded_bytes = block_values * FVM_SWF_SOUND_PCM_BYTES;
        if (decoded_bytes > (size_t)-1 - snapshot->peak_bytes ||
            (budget && decoded_bytes > *budget - snapshot->peak_bytes))
        {
            delete snapshot;
            return FVM_OUT_OF_MEMORY;
        }
        snapshot->peak_bytes += decoded_bytes;
        used += decoded_bytes;
        values += block_values;
        entry.block.sample_count = frames;
        entry.block.sample_count_known = 1;
    }
    try
    {
        snapshot->pcm.resize(values);
        size_t offset = 0;
        for (size_t i = 0; i < snapshot->blocks.size(); ++i)
        {
            FVM_SWFStreamSoundEntry &entry = snapshot->blocks[i];
            FVM_SWFSoundInfo info;
            std::memset(&info, 0, sizeof(info));
            info.format = FVM_SWF_SOUND_ADPCM;
            info.sample_rate = snapshot->info.sample_rate;
            info.bits_per_sample = FVM_SWF_SOUND_BITS_16;
            info.channels = snapshot->info.channels;
            info.sample_count = (unsigned long)entry.block.sample_count;
            info.data =
                snapshot->bytes.empty() ? &snapshot->empty : snapshot->bytes.begin() + entry.offset;
            info.data_size = entry.block.data_size;
            size_t written = 0;
            /* A genuinely empty block has no width prefix for the ordinary DefineSound decoder. */
            result = !info.data_size
                         ? FVM_OK
                         : fvm_swf_sound_decode_pcm(info, 0, info.sample_count,
                                                    values ? snapshot->pcm.begin() + offset : 0,
                                                    values - offset, &written);
            if (result != FVM_OK)
            {
                delete snapshot;
                return result;
            }
            entry.offset = offset * FVM_SWF_SOUND_PCM_BYTES;
            entry.block.data_size = written * FVM_SWF_SOUND_PCM_BYTES;
            offset += written;
        }
        snapshot->retained_bytes = snapshot->peak_bytes - snapshot->bytes.size();
        fvm_detail::vector<unsigned char> encoded;
        snapshot->bytes.swap(
            encoded); /* Release compressed storage when this preparation returns. */
        snapshot->info.format = FVM_SWF_SOUND_PCM_NATIVE;
        snapshot->info.bits_per_sample = FVM_SWF_SOUND_BITS_16;
        *out = snapshot;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        delete snapshot;
        return FVM_OUT_OF_MEMORY;
    }
}
/* [GC:] Legacy preparation keeps its payload-only runtime limit, with no extra caller ceiling. */
extern "C" FVM_Result fvm_swf_stream_sound_snapshot_open_pcm(FVM_Context *context, size_t stream,
                                                             FVM_SWFStreamSoundSnapshot **out)
{
    return snapshot_open_pcm(context, stream, 0, 0, 0, false, out);
}
/* [GC:] Bound both initial copying and compressed expansion before allocating either payload. */
extern "C" FVM_Result
fvm_swf_stream_sound_snapshot_open_pcm_bounded(FVM_Context *context, size_t stream, size_t budget,
                                               FVM_SWFStreamSoundSnapshot **out)
{
    return snapshot_open_pcm(context, stream, &budget, 0, 0, false, out);
}
/* [GC:] Explicit inferred policy adds uniquely byte-delimited partial ADPCM to bounded PCM.
 * Preparation owns no managed references after return and runs before native worker publication. */
extern "C" FVM_Result
fvm_swf_stream_sound_snapshot_open_pcm_inferred(FVM_Context *context, size_t stream, size_t budget,
                                                FVM_SWFStreamSoundSnapshot **out)
{
    return snapshot_open_pcm(context, stream, &budget, 0, 0, true, out);
}
/* [GC:] Host-authoritative block counts allow partial ADPCM packets without a runtime dependency.
 * The count table is borrowed only during this synchronous owner-side bounded preparation. */
extern "C" FVM_Result
fvm_swf_stream_sound_snapshot_open_adpcm_counted(FVM_Context *context, size_t stream,
                                                 const size_t *counts, size_t count, size_t budget,
                                                 FVM_SWFStreamSoundSnapshot **out)
{
    return snapshot_open_pcm(context, stream, &budget, counts, &count, false, out);
}
/* Copy native allocation charges only; this immutable observation needs no original VM. */
extern "C" FVM_Result
fvm_swf_stream_sound_snapshot_storage_info(const FVM_SWFStreamSoundSnapshot *snapshot,
                                           FVM_SWFStreamSoundStorageInfo *out)
{
    if (!snapshot || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    FVM_SWFStreamSoundStorageInfo info;
    info.struct_size = sizeof(info);
    info.retained_bytes = snapshot->retained_bytes;
    info.peak_bytes = snapshot->peak_bytes;
    *out = info;
    return FVM_OK;
}
/* Copy source geometry without consulting the original context. */
extern "C" FVM_Result fvm_swf_stream_sound_snapshot_info(const FVM_SWFStreamSoundSnapshot *snapshot,
                                                         FVM_SWFStreamSoundInfo *out)
{
    if (!snapshot || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    *out = snapshot->info;
    return FVM_OK;
}
/* Return the indexed block count without scanning tags. */
extern "C" FVM_Result
fvm_swf_stream_sound_snapshot_block_count(const FVM_SWFStreamSoundSnapshot *snapshot, size_t *out)
{
    if (!snapshot || !out)
        return FVM_INVALID_ARGUMENT;
    *out = snapshot->blocks.size();
    return FVM_OK;
}
/* Reconstruct an owned raw-payload view in constant time. */
extern "C" FVM_Result
fvm_swf_stream_sound_snapshot_block_at(const FVM_SWFStreamSoundSnapshot *snapshot, size_t ordinal,
                                       FVM_SWFStreamSoundBlock *out)
{
    if (!snapshot || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    if (ordinal >= snapshot->blocks.size())
        return FVM_NOT_FOUND;
    const FVM_SWFStreamSoundEntry &entry = snapshot->blocks[ordinal];
    *out = entry.block;
    if (!snapshot->pcm.empty())
        out->data = reinterpret_cast<const unsigned char *>(snapshot->pcm.begin()) + entry.offset;
    else
        out->data =
            snapshot->bytes.empty() ? &snapshot->empty : snapshot->bytes.begin() + entry.offset;
    return FVM_OK;
}
/* Inspect an owned compressed block without consulting the originating VM. */
extern "C" FVM_Result
fvm_swf_stream_sound_snapshot_mp3_block_at(const FVM_SWFStreamSoundSnapshot *snapshot,
                                           size_t ordinal, FVM_SWFStreamSoundMP3Block *out)
{
    if (!snapshot || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    FVM_SWFStreamSoundBlock block;
    block.struct_size = sizeof(block);
    FVM_Result result = fvm_swf_stream_sound_snapshot_block_at(snapshot, ordinal, &block);
    return result == FVM_OK ? mp3_block(snapshot->info, block, out) : result;
}
/* Decode only the requested range from the indexed source; no allocation, callback or VM access. */
extern "C" FVM_Result
fvm_swf_stream_sound_snapshot_read_pcm(const FVM_SWFStreamSoundSnapshot *snapshot, size_t ordinal,
                                       size_t first, size_t frames, short *out, size_t capacity,
                                       size_t *written)
{
    if (!written)
        return FVM_INVALID_ARGUMENT;
    *written = 0;
    FVM_SWFStreamSoundBlock block;
    block.struct_size = sizeof(block);
    FVM_Result result = fvm_swf_stream_sound_snapshot_block_at(snapshot, ordinal, &block);
    if (result != FVM_OK)
        return result;
    if (snapshot->info.format != FVM_SWF_SOUND_PCM_NATIVE &&
        snapshot->info.format != FVM_SWF_SOUND_PCM_LE)
        return FVM_UNSUPPORTED;
    if (first > block.sample_count || block.sample_count > ULONG_MAX)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFSoundInfo info;
    std::memset(&info, 0, sizeof(info));
    info.format = snapshot->info.format;
    info.sample_rate = snapshot->info.sample_rate;
    info.bits_per_sample = snapshot->info.bits_per_sample;
    info.channels = snapshot->info.channels;
    info.sample_count = (unsigned long)block.sample_count;
    info.data = block.data;
    info.data_size = block.data_size;
    return fvm_swf_sound_decode_pcm(info, (unsigned long)first, frames, out, capacity, written);
}
/* Release only native owned storage; null is accepted and no originating VM is required. */
extern "C" void fvm_swf_stream_sound_snapshot_close(FVM_SWFStreamSoundSnapshot *snapshot)
{
    delete snapshot;
}
