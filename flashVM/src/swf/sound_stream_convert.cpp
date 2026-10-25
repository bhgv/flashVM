/* Explicit bounded interpolation with rational source phase carried across encoded PCM blocks. */
#include "flashvm/swf_stream_convert.h"
#include "sound_constants.h"
#include <cstring>


/* Descriptors are private after init; validate before any indexing or rate arithmetic. */
static bool convert_state(const FVM_SWFStreamConverter *s)
{
    return s && s->struct_size == sizeof(*s) && s->snapshot && s->block_index <= s->block_count &&
           s->output_rate >= FVM_STREAM_CONVERT_MIN_RATE && s->output_rate <= FVM_STREAM_CONVERT_MAX_RATE &&
           s->phase < s->output_rate &&
           (s->output_channels == FVM_SWF_SOUND_MONO_CHANNELS || s->output_channels == FVM_SWF_SOUND_STEREO_CHANNELS);
}


/* [GC:] Reuse PCM validation while borrowing only copied native source storage, never a VM. */
extern "C" FVM_Result fvm_swf_stream_converter_init(FVM_SWFStreamConverter *out,
                                                    const FVM_SWFStreamSoundSnapshot *snapshot,
                                                    unsigned long rate,
                                                    unsigned int channels)
{
    if (!out || rate < FVM_STREAM_CONVERT_MIN_RATE || rate > FVM_STREAM_CONVERT_MAX_RATE ||
        (channels != FVM_SWF_SOUND_MONO_CHANNELS && channels != FVM_SWF_SOUND_STEREO_CHANNELS))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_SWFStreamPCMReader reader;
    FVM_Result result = fvm_swf_stream_pcm_reader_init(&reader, snapshot);
    if (result != FVM_OK)
    {
        return result;
    }
    FVM_SWFStreamConverter next;
    std::memset(&next, 0, sizeof(next));
    next.struct_size = sizeof(next);
    next.snapshot = snapshot;
    next.info = reader.info;
    next.block_count = reader.block_count;
    next.output_rate = rate;
    next.output_channels = channels;
    *out = next;
    return FVM_OK;
}


/* Select continuous segment interpolation while preserving legacy initialization semantics. */
extern "C" FVM_Result fvm_swf_stream_converter_init_continuous(FVM_SWFStreamConverter *out,
                                                               const FVM_SWFStreamSoundSnapshot *snapshot,
                                                               unsigned long rate,
                                                               unsigned int channels)
{
    FVM_Result result = fvm_swf_stream_converter_init(out, snapshot, rate, channels);
    if (result == FVM_OK)
    {
        out->continuous = 1;
    }
    return result;
}


/* Resolve the next source frame once per call, skipping empty metadata without advancing state. */
static FVM_Result convert_neighbor(const FVM_SWFStreamConverter &s, short *raw, bool *found)
{
    *found = false;
    for (size_t ordinal = s.block_index + 1; ordinal < s.block_count; ++ordinal)
    {
        FVM_SWFStreamSoundBlock block;
        block.struct_size = sizeof(block);
        FVM_Result result = fvm_swf_stream_sound_snapshot_block_at(s.snapshot, ordinal, &block);
        if (result != FVM_OK)
        {
            return result;
        }
        if (!block.sample_count_known)
        {
            return FVM_UNSUPPORTED;
        }
        if (!block.sample_count)
        {
            continue;
        }
        size_t written;
        result = fvm_swf_stream_sound_snapshot_read_pcm(
            s.snapshot, ordinal, 0, 1, raw, FVM_SWF_SOUND_STEREO_CHANNELS, &written);
        if (result == FVM_OK)
        {
            *found = true;
        }
        return result;
    }
    return FVM_OK;
}


/* Advance one exact rational output step without multiplying source lengths by output rates. */
static bool convert_step(FVM_SWFStreamConverter &s, size_t total)
{
    unsigned long phase = s.phase + s.info.sample_rate;
    size_t step = (size_t)(phase / s.output_rate);
    s.phase = phase % s.output_rate;
    if (step >= total - s.first)
    {
        s.first = step - (total - s.first);
        ++s.block_index;
        s.started = 0;
        return true;
    }
    s.first += step;
    s.started = 1;
    return false;
}


/* Map source mono/stereo into one chosen channel before interpolation, with explicit rounding. */
static long convert_channel(const short *p, unsigned int source, unsigned int output, unsigned int channel)
{
    if (output == FVM_SWF_SOUND_STEREO_CHANNELS)
    {
        return p[source == FVM_SWF_SOUND_MONO_CHANNELS ? 0 : channel];
    }
    if (source == FVM_SWF_SOUND_MONO_CHANNELS)
    {
        return p[0];
    }
    long sum = (long)p[0] + p[1];
    return sum < 0 ? -((-sum) / (long)FVM_SWF_SOUND_STEREO_CHANNELS) : sum / (long)FVM_SWF_SOUND_STEREO_CHANNELS;
}


/* Plan the bounded output first; immutable PCM validation ensures decode cannot fail mid-write. */
extern "C" FVM_Result fvm_swf_stream_converter_next(FVM_SWFStreamConverter *s,
                                                    size_t maximum,
                                                    short *pcm,
                                                    size_t capacity,
                                                    FVM_SWFStreamConvertedChunk *out,
                                                    size_t *written)
{
    if (!convert_state(s) || !maximum || !out || out->struct_size != sizeof(*out) || !written)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (s->block_index == s->block_count)
    {
        return FVM_NOT_FOUND;
    }
    FVM_SWFStreamSoundBlock block;
    block.struct_size = sizeof(block);
    FVM_Result result = fvm_swf_stream_sound_snapshot_block_at(s->snapshot, s->block_index, &block);
    if (result != FVM_OK)
    {
        return result;
    }
    if (!block.sample_count_known)
    {
        return FVM_UNSUPPORTED;
    }
    if (maximum > FVM_STREAM_CONVERT_MAX_FRAMES)
    {
        maximum = FVM_STREAM_CONVERT_MAX_FRAMES;
    }
    FVM_SWFStreamConverter next = *s;
    size_t count = 0;
    bool end = false;
    if (next.first >= block.sample_count)
    {
        next.first -= block.sample_count;
        ++next.block_index;
        next.started = 0;
        end = true;
    }
    else
    {
        do
        {
            ++count;
            end = convert_step(next, block.sample_count);
        } while (!end && count < maximum);
    }
    if (count > capacity / s->output_channels || (count && !pcm))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_SWFStreamConvertedChunk chunk;
    std::memset(&chunk, 0, sizeof(chunk));
    chunk.struct_size = sizeof(chunk);
    chunk.stream_index = s->info.stream_index;
    chunk.block_index = s->block_index;
    chunk.frame = block.frame;
    chunk.first_source = s->first < block.sample_count ? s->first : block.sample_count;
    chunk.phase = s->phase;
    chunk.frames = count;
    chunk.sample_rate = s->output_rate;
    chunk.channels = s->output_channels;
    if (!s->started)
    {
        chunk.flags |= FVM_STREAM_CONVERT_BEGIN;
    }
    if (end)
    {
        chunk.flags |= FVM_STREAM_CONVERT_END;
    }
    if (!block.sample_count)
    {
        chunk.flags |= FVM_STREAM_CONVERT_EMPTY;
    }
    short neighbor[FVM_SWF_SOUND_STEREO_CHANNELS];
    bool have_neighbor = false;
    /* Resolve before writing any output, and only when this chunk reaches the final interval. */
    if (count && s->continuous && (end || next.first == block.sample_count - 1))
    {
        result = convert_neighbor(*s, neighbor, &have_neighbor);
        if (result != FVM_OK)
        {
            return result;
        }
    }
    FVM_SWFStreamConverter position = *s;
    for (size_t i = 0; i < count; ++i)
    {
        short raw[2 * FVM_SWF_SOUND_STEREO_CHANNELS];
        size_t values;
        size_t frames = block.sample_count - position.first > 1 ? 2 : 1;
        result = fvm_swf_stream_sound_snapshot_read_pcm(
            s->snapshot, s->block_index, position.first, frames, raw, 2 * FVM_SWF_SOUND_STEREO_CHANNELS, &values);
        if (result != FVM_OK)
        {
            return result; /* Unreachable for a live immutable validated PCM source. */
        }
        for (unsigned int channel = 0; channel < s->output_channels; ++channel)
        {
            long a = convert_channel(raw, s->info.channels, s->output_channels, channel);
            const short *after = frames == 1 ? (have_neighbor ? neighbor : raw) : raw + s->info.channels;
            long b = convert_channel(after, s->info.channels, s->output_channels, channel);
            /* Products are exact below 2^53; avoid overflowing 32-bit long on LLP64/ILP32. */
            double value =
                ((double)a * (s->output_rate - position.phase) + (double)b * position.phase) / s->output_rate;
            pcm[i * s->output_channels + channel] = (short)value;
        }
        convert_step(position, block.sample_count);
    }
    *s = next;
    *out = chunk;
    *written = count * s->output_channels;
    return FVM_OK;
}


/* Seeking explicitly starts a fresh rational grid instead of pretending to seek device state. */
extern "C" FVM_Result fvm_swf_stream_converter_seek(FVM_SWFStreamConverter *s, size_t ordinal, size_t first)
{
    if (!convert_state(s))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_SWFStreamPCMReader r;
    FVM_Result result = fvm_swf_stream_pcm_reader_init(&r, s->snapshot);
    if (result == FVM_OK)
    {
        result = fvm_swf_stream_pcm_reader_seek(&r, ordinal, first);
    }
    if (result != FVM_OK)
    {
        return result;
    }
    s->block_index = ordinal;
    s->first = first;
    s->phase = 0;
    s->started = 0;
    return FVM_OK;
}
