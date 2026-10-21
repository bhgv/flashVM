/* Whole-packet ADPCM preparation: independent bit fixtures and post-VM native reads. */
#include "swf_sound_fixture.hpp"
#include "flashvm/swf_stream_pcm.h"
#include "../src/core/runtime.hpp"
#include <cstdio>
using namespace sound_fixture;
struct Bits
{
    Bytes bytes;
    unsigned n;
    Bits() : n(0)
    {
    }
    void put(unsigned value, unsigned width)
    {
        for (unsigned i = width; i; --i)
        {
            if (!(n % 8))
                bytes.push_back(0);
            bytes[bytes.size() - 1] |= ((value >> (i - 1)) & 1) << (7 - n % 8);
            ++n;
        }
    }
};
static Bytes swf_block(const Bytes &payload, unsigned channels)
{
    Bytes tags, head, empty;
    head.push_back(0x0e);
    head.push_back(0x1e | (channels - 1));
    word(head, 1); /* Deliberately misleading average: never use it as a decoded count. */
    tag(tags, 18, head);
    tag(tags, 19, payload);
    return movie(tags);
}
static void verify(FVM_SWFStreamSoundSnapshot *snapshot, unsigned channels, unsigned width)
{
    FVM_SWFStreamSoundInfo info;
    info.struct_size = sizeof(info);
    assert(fvm_swf_stream_sound_snapshot_info(snapshot, &info) == FVM_OK);
    assert(info.format == FVM_SWF_SOUND_PCM_NATIVE && info.bits_per_sample == 16 &&
           info.average_samples == 1);
    FVM_SWFStreamPCMReader reader;
    assert(fvm_swf_stream_pcm_reader_init(&reader, snapshot) == FVM_OK);
    size_t total = 0, written = 0;
    short pcm[34];
    FVM_SWFStreamPCMChunk chunk;
    for (;;)
    {
        chunk.struct_size = sizeof(chunk);
        FVM_Result r = fvm_swf_stream_pcm_reader_next(&reader, 17, pcm, 34, &chunk, &written);
        if (r == FVM_NOT_FOUND)
            break;
        assert(r == FVM_OK && written == chunk.frames * channels);
        for (size_t f = 0; f < chunk.frames; ++f)
            for (unsigned ch = 0; ch < channels; ++ch)
            {
                long expected = ch ? -100 : 100;
                /* Zero-magnitude codes at step index zero add floor(7 / 2^(width-1)). */
                long delta = 7 / (1U << (width - 1));
                expected += delta * ((total + f) % 4096);
                if (expected > 32767)
                    expected = 32767;
                assert(pcm[f * channels + ch] == expected);
            }
        total += chunk.frames;
    }
    assert(total == 8192);
}
static int export_file(const char *input, const char *output)
{
    FILE *file = std::fopen(input, "rb");
    if (!file)
        return 2;
    Bytes bytes;
    int ch;
    while ((ch = std::fgetc(file)) != EOF)
        bytes.push_back((unsigned char)ch);
    std::fclose(file);
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_SWFStreamSoundSnapshot *snapshot = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
           fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_load_memory(c, bytes.begin(), bytes.size()) == FVM_OK);
    FVM_Result result = fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &snapshot);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    if (result != FVM_OK)
    {
        assert(!snapshot);
        return result == FVM_UNSUPPORTED ? 3 : 4;
    }
    FVM_SWFStreamPCMReader reader;
    assert(fvm_swf_stream_pcm_reader_init(&reader, snapshot) == FVM_OK);
    file = std::fopen(output, "wb");
    assert(file);
    short pcm[62];
    size_t written;
    FVM_SWFStreamPCMChunk chunk;
    for (;;)
    {
        chunk.struct_size = sizeof(chunk);
        result = fvm_swf_stream_pcm_reader_next(&reader, 31, pcm, 62, &chunk, &written);
        if (result == FVM_NOT_FOUND)
            break;
        assert(result == FVM_OK);
        assert(std::fwrite(pcm, sizeof(short), written, file) == written);
    }
    assert(!std::fclose(file));
    fvm_swf_stream_sound_snapshot_close(snapshot);
    return 0;
}
int main(int argc, char **argv)
{
    if (argc == 3)
        return export_file(argv[1], argv[2]);
    for (unsigned width = 2; width <= 5; ++width)
        for (unsigned channels = 1; channels <= 2; ++channels)
        {
            Bits bits;
            bits.put(width - 2, 2);
            for (unsigned f = 0; f < 8192; ++f)
                for (unsigned ch = 0; ch < channels; ++ch)
                {
                    if (!(f % 4096))
                    {
                        bits.put(ch ? 65436 : 100, 16);
                        bits.put(0, 6);
                    }
                    else
                        bits.put(0, width);
                }
            FVM_Config cfg;
            FVM_Runtime *rt = 0;
            FVM_Context *c = 0;
            assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
                   fvm_context_create(rt, &c) == FVM_OK);
            Bytes swf = swf_block(bits.bytes, channels);
            assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK);
            FVM_SWFStreamSoundSnapshot *snapshot = 0, *raw = 0;
            assert(fvm_swf_stream_sound_snapshot_open(c, 0, &raw) == FVM_OK);
            FVM_SWFStreamSoundBlock block;
            block.struct_size = sizeof(block);
            assert(fvm_swf_stream_sound_snapshot_block_at(raw, 0, &block) == FVM_OK &&
                   !block.sample_count_known && block.data_size == bits.bytes.size());
            assert(fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &snapshot) == FVM_OK);
            fvm_swf_stream_sound_snapshot_close(raw);
            size_t old = c->runtime->config.memory_limit_bytes;
            c->runtime->config.memory_limit_bytes = swf.size() + 1024;
            raw = reinterpret_cast<FVM_SWFStreamSoundSnapshot *>(1);
            assert(fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &raw) == FVM_OUT_OF_MEMORY && !raw);
            c->runtime->config.memory_limit_bytes = old;
            Bytes bad = bits.bytes;
            bad[bad.size() - 1] |= 1;
            swf = swf_block(bad, channels);
            assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK);
            if (bits.n % 8)
                assert(fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &raw) == FVM_INVALID_ARGUMENT &&
                       !raw);
            bad = bits.bytes;
            bad.resize(bad.size() - 1);
            swf = swf_block(bad, channels);
            assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK);
            assert(fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &raw) == FVM_UNSUPPORTED && !raw);
            fvm_context_destroy(c);
            fvm_runtime_destroy(rt);
            verify(snapshot, channels, width);
            fvm_swf_stream_sound_snapshot_close(snapshot);
        }
    return 0;
}
