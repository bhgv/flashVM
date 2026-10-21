/* Bounded MP3 stream preparation: persistent reservoir, one initial latency trim, immutable native PCM. */
#include "sound_stream_internal.hpp"
#include "sound_stream_constants.h"
#include "sound_mp3.hpp"
#include "sound_constants.h"
#include <cstring>
#include <new>

/* Read encoded bytes explicitly: the second pass already owns a nonempty decoded PCM vector. */
static FVM_Result encoded_payload(const FVM_SWFStreamSoundSnapshot *snapshot, size_t ordinal,
                                    FVM_SWFStreamSoundMP3Block *out)
{
    const FVM_SWFStreamSoundEntry &entry = snapshot->blocks[ordinal];
    if (entry.offset > snapshot->bytes.size() ||
        entry.block.data_size > snapshot->bytes.size() - entry.offset ||
        entry.block.data_size < FVM_SWF_SOUND_STREAM_MP3_PREFIX_BYTES)
        return FVM_INVALID_ARGUMENT;
    const unsigned char *data = snapshot->bytes.begin() + entry.offset;
    out->declared_samples = (unsigned int)data[0] | ((unsigned int)data[1] << FVM_SWF_SOUND_BYTE_BITS);
    unsigned long seek = (unsigned long)data[2] | ((unsigned long)data[3] << FVM_SWF_SOUND_BYTE_BITS);
    out->seek_samples = (int)(seek >= FVM_SWF_SOUND_SIGN_BIT ? (long)seek - FVM_SWF_SOUND_SIGN_RANGE : (long)seek);
    out->data = data + FVM_SWF_SOUND_STREAM_MP3_PREFIX_BYTES;
    out->data_size = entry.block.data_size - FVM_SWF_SOUND_STREAM_MP3_PREFIX_BYTES;
    return FVM_OK;
}
/* Run one complete native pass; only the second pass writes into already bounded PCM storage. */
static FVM_Result mp3_stream_pass(FVM_SWFStreamSoundSnapshot *snapshot, size_t limit,
                                   const size_t *budget, bool store, size_t *value_count)
{
    FVM_SWFMP3Decoder decoder;
    size_t skip = (size_t)snapshot->info.latency_seek;
    size_t used = snapshot->bytes.size() + snapshot->blocks.size() * sizeof(FVM_SWFStreamSoundEntry);
    size_t values = 0;
    unsigned int rate = 0, channels = 0;
    for (size_t i = 0; i < snapshot->blocks.size(); ++i)
    {
        FVM_SWFStreamSoundMP3Block payload;
        payload.struct_size = sizeof(payload);
        FVM_Result result = encoded_payload(snapshot, i, &payload);
        if (result != FVM_OK)
            return result;
        if (!i && payload.seek_samples != snapshot->info.latency_seek)
            return FVM_INVALID_ARGUMENT;
        size_t at = 0, decoded = 0, copied = 0;
        size_t start = values;
        while (at < payload.data_size)
        {
            FVM_SWFMP3Frame packet;
            result = decoder.decode(payload.data + at, payload.data_size - at, &packet);
            if (result != FVM_OK)
                return result;
            if (!rate)
            {
                rate = packet.rate;
                channels = packet.channels;
            }
            if (packet.rate != rate || packet.channels != channels ||
                decoded > payload.declared_samples || packet.samples > payload.declared_samples - decoded)
                return FVM_INVALID_ARGUMENT;
            size_t discard = skip < packet.samples ? skip : packet.samples;
            skip -= discard;
            size_t frames = packet.samples - discard;
            size_t packet_values = frames * channels;
            if (packet_values > (size_t)-1 - values)
                return FVM_OUT_OF_MEMORY;
            if (store && packet_values)
            {
                if (values > snapshot->pcm.size() || packet_values > snapshot->pcm.size() - values)
                    return FVM_ERROR;
                std::memcpy(snapshot->pcm.begin() + values, decoder.pcm + discard * channels,
                            packet_values * sizeof(short));
            }
            values += packet_values;
            copied += frames;
            decoded += packet.samples;
            at += packet.bytes;
        }
        if (decoded != payload.declared_samples)
            return FVM_INVALID_ARGUMENT;
        size_t block_values = values - start;
        if (!store)
        {
            if (block_values > (limit - used) / FVM_SWF_SOUND_PCM_BYTES)
                return FVM_OUT_OF_MEMORY;
            size_t bytes = block_values * FVM_SWF_SOUND_PCM_BYTES;
            if (bytes > (size_t)-1 - snapshot->peak_bytes ||
                (budget && bytes > *budget - snapshot->peak_bytes))
                return FVM_OUT_OF_MEMORY;
            snapshot->peak_bytes += bytes;
            used += bytes;
            snapshot->blocks[i].block.sample_count = copied;
            snapshot->blocks[i].block.sample_count_known = 1;
        }
        else
        {
            snapshot->blocks[i].offset = start * FVM_SWF_SOUND_PCM_BYTES;
            snapshot->blocks[i].block.data_size = block_values * FVM_SWF_SOUND_PCM_BYTES;
        }
    }
    if (skip)
        return FVM_INVALID_ARGUMENT;
    if (rate)
    {
        snapshot->info.sample_rate = rate;
        snapshot->info.channels = channels;
    }
    *value_count = values;
    return FVM_OK;
}
/* [GC:] Consume independent native ownership only; no context, managed pointer or callback survives. */
FVM_Result fvm_swf_stream_prepare_mp3(FVM_SWFStreamSoundSnapshot *snapshot, size_t limit,
                                      const size_t *budget, FVM_SWFStreamSoundSnapshot **out)
{
    if (snapshot->info.latency_seek < 0)
    {
        delete snapshot;
        return FVM_UNSUPPORTED;
    }
    size_t values = 0;
    FVM_Result result = mp3_stream_pass(snapshot, limit, budget, false, &values);
    if (result != FVM_OK)
    {
        delete snapshot;
        return result;
    }
    try
    {
        snapshot->pcm.resize(values);
        size_t written = 0;
        result = mp3_stream_pass(snapshot, limit, budget, true, &written);
        if (result != FVM_OK || written != values)
        {
            delete snapshot;
            return result != FVM_OK ? result : FVM_ERROR;
        }
        snapshot->retained_bytes = snapshot->peak_bytes - snapshot->bytes.size();
        fvm_detail::vector<unsigned char> encoded;
        snapshot->bytes.swap(encoded);
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
