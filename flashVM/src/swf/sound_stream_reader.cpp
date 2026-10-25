/* Native frame-aware prepared PCM cursor; no VM lifetime or audio-device scheduling. */
#include "flashvm/swf_stream_pcm.h"
#include "sound_constants.h"
#include <climits>
#include <cstring>


/* Private cursor fields are initialized together and must not be modified by the host. */
static bool stream_reader(const FVM_SWFStreamPCMReader *r)
{
    return r && r->struct_size == sizeof(*r) && r->snapshot && r->block_index <= r->block_count &&
           (r->info.channels == FVM_SWF_SOUND_MONO_CHANNELS || r->info.channels == FVM_SWF_SOUND_STEREO_CHANNELS);
}


/* [GC:] Borrow only an independent immutable native snapshot, never a context or managed root. */
extern "C" FVM_Result fvm_swf_stream_pcm_reader_init(FVM_SWFStreamPCMReader *out,
                                                     const FVM_SWFStreamSoundSnapshot *snapshot)
{
    FVM_SWFStreamPCMReader next;
    if (!out || !snapshot)
    {
        return FVM_INVALID_ARGUMENT;
    }
    std::memset(&next, 0, sizeof(next));
    next.struct_size = sizeof(next);
    next.info.struct_size = sizeof(next.info);
    FVM_Result result = fvm_swf_stream_sound_snapshot_info(snapshot, &next.info);
    if (result != FVM_OK)
    {
        return result;
    }
    if ((next.info.format != FVM_SWF_SOUND_PCM_NATIVE && next.info.format != FVM_SWF_SOUND_PCM_LE) ||
        sizeof(short) != FVM_SWF_SOUND_PCM_BYTES || SHRT_MAX != FVM_SWF_SOUND_SHORT_MAX ||
        SHRT_MIN != FVM_SWF_SOUND_SHORT_MIN)
    {
        return FVM_UNSUPPORTED;
    }
    result = fvm_swf_stream_sound_snapshot_block_count(snapshot, &next.block_count);
    if (result != FVM_OK)
    {
        return result;
    }
    next.snapshot = snapshot;
    *out = next;
    return FVM_OK;
}


/* Preserve frame/block boundaries, including empty blocks and explicit end-offset seeks. */
extern "C" FVM_Result fvm_swf_stream_pcm_reader_next(
    FVM_SWFStreamPCMReader *r, size_t maximum, short *pcm, size_t capacity, FVM_SWFStreamPCMChunk *out, size_t *written)
{
    if (!stream_reader(r) || !maximum || !out || out->struct_size != sizeof(*out) || !written)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (r->block_index == r->block_count)
    {
        return FVM_NOT_FOUND;
    }
    FVM_SWFStreamSoundBlock block;
    block.struct_size = sizeof(block);
    FVM_Result result = fvm_swf_stream_sound_snapshot_block_at(r->snapshot, r->block_index, &block);
    if (result != FVM_OK)
    {
        return result;
    }
    if (!block.sample_count_known || r->first > block.sample_count)
    {
        return FVM_INVALID_ARGUMENT;
    }
    size_t frames = block.sample_count - r->first;
    if (frames > maximum)
    {
        frames = maximum;
    }
    if (frames > capacity / r->info.channels || (frames && !pcm))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_SWFStreamPCMChunk chunk;
    std::memset(&chunk, 0, sizeof(chunk));
    chunk.struct_size = sizeof(chunk);
    chunk.stream_index = r->info.stream_index;
    chunk.block_index = r->block_index;
    chunk.frame = block.frame;
    chunk.first = r->first;
    chunk.frames = frames;
    chunk.sample_rate = r->info.sample_rate;
    chunk.channels = r->info.channels;
    if (!r->first)
    {
        chunk.flags |= FVM_STREAM_PCM_BEGIN;
    }
    if (frames == block.sample_count - r->first)
    {
        chunk.flags |= FVM_STREAM_PCM_END;
    }
    if (!block.sample_count)
    {
        chunk.flags |= FVM_STREAM_PCM_EMPTY;
    }
    size_t values;
    result =
        fvm_swf_stream_sound_snapshot_read_pcm(r->snapshot, r->block_index, r->first, frames, pcm, capacity, &values);
    if (result != FVM_OK)
    {
        return result;
    }
    if (chunk.flags & FVM_STREAM_PCM_END)
    {
        ++r->block_index;
        r->first = 0;
    }
    else
    {
        r->first += frames;
    }
    *out = chunk;
    *written = values;
    return FVM_OK;
}


/* Position changes need no source decoding and no output-device rollback. */
extern "C" FVM_Result fvm_swf_stream_pcm_reader_seek(FVM_SWFStreamPCMReader *r, size_t ordinal, size_t first)
{
    if (!stream_reader(r) || ordinal > r->block_count)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (ordinal == r->block_count)
    {
        if (first)
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    else
    {
        FVM_SWFStreamSoundBlock block;
        block.struct_size = sizeof(block);
        FVM_Result result = fvm_swf_stream_sound_snapshot_block_at(r->snapshot, ordinal, &block);
        if (result != FVM_OK)
        {
            return result;
        }
        if (!block.sample_count_known || first > block.sample_count)
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    r->block_index = ordinal;
    r->first = first;
    return FVM_OK;
}
