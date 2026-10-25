/* Native continuous MP3 preparation: persistent bit reservoir, one latency trim and exact block counts. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_fixture.hpp"
#include "swf_mp3_fixture.hpp"
#include "../src/swf/sound_mp3.hpp"
#include "flashvm/swf_stream_pcm.h"
#include <cstdlib>

using namespace sound_fixture;


/* Independent frame grouping puts each MPEG frame in its own movie block with empty markers. */
static Bytes
stream_movie(const mp3_fixture::Fixture &fixture, unsigned latency, bool mismatch = false, bool truncate = false)
{
    Bytes tags, head, empty;
    head.push_back(14);
    head.push_back(46);
    word(head, 1);
    word(head, latency);
    tag(tags, 18, head);
    size_t at = 0;
    unsigned count = 0;
    while (at < fixture.size)
    {
        FVM_SWFMP3Frame packet;
        if (!(fvm_swf_mp3_probe(fixture.data + at, fixture.size - at, &packet) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes payload;
        word(payload, packet.samples + (mismatch && !at ? 1 : 0));
        word(payload, !at ? latency : 37); /* Later seek offsets must not trim continuous audio. */
        size_t bytes = packet.bytes - (truncate && at + packet.bytes == fixture.size ? 1 : 0);
        for (size_t i = 0; i < bytes; ++i)
        {
            payload.push_back(fixture.data[at + i]);
        }
        tag(tags, 19, payload);
        tag(tags, 1, empty);
        ++count;
        if (!at)
        {
            payload.clear();
            word(payload, 0);
            word(payload, 0);
            tag(tags, 19, payload);
            tag(tags, 1, empty);
            ++count;
        }
        at += packet.bytes;
    }
    tag(tags, 0, empty);
    Bytes swf;
    swf.push_back('F');
    swf.push_back('W');
    swf.push_back('S');
    swf.push_back(8);
    integer(swf, tags.size() + 14);
    swf.push_back(8);
    swf.push_back(0);
    word(swf, 12 * 256);
    word(swf, count);
    for (size_t i = 0; i < tags.size(); ++i)
    {
        swf.push_back(tags[i]);
    }
    return swf;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (size_t f = 0; f < sizeof(mp3_fixture::fixtures) / sizeof(mp3_fixture::fixtures[0]); ++f)
    {
        const mp3_fixture::Fixture &fixture = mp3_fixture::fixtures[f];
        for (unsigned latency = 0; latency <= 1105; latency += 1105)
        {
            Bytes swf = stream_movie(fixture, latency);
            if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_SWFStreamSoundSnapshot *source = 0, *exact = 0;
            if (!(fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &source) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_SWFStreamSoundInfo info;
            info.struct_size = sizeof(info);
            if (!(fvm_swf_stream_sound_snapshot_info(source, &info) == FVM_OK &&
                  info.format == FVM_SWF_SOUND_PCM_NATIVE && info.sample_rate == fixture.rate &&
                  info.channels == fixture.channels && info.average_samples == 1 && info.latency_seek == (int)latency))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_SWFStreamSoundStorageInfo storage;
            storage.struct_size = sizeof(storage);
            if (!(fvm_swf_stream_sound_snapshot_storage_info(source, &storage) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, storage.peak_bytes, &exact) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_swf_stream_sound_snapshot_close(exact);
            if (!(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, storage.peak_bytes - 1, &exact) ==
                      FVM_OUT_OF_MEMORY &&
                  !exact))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_SWFStreamPCMReader reader;
            if (!(fvm_swf_stream_pcm_reader_init(&reader, source) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            short pcm[62];
            size_t total = 0, written = 0;
            for (;;)
            {
                FVM_SWFStreamPCMChunk chunk;
                chunk.struct_size = sizeof(chunk);
                FVM_Result r = fvm_swf_stream_pcm_reader_next(&reader, 31, pcm, 62, &chunk, &written);
                if (r == FVM_NOT_FOUND)
                {
                    break;
                }
                if (!(r == FVM_OK && written == chunk.frames * fixture.channels))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (size_t p = 0; p < sizeof(mp3_fixture::positions) / sizeof(mp3_fixture::positions[0]); ++p)
                {
                    size_t position = mp3_fixture::positions[p];
                    if (position < latency)
                    {
                        continue;
                    }
                    position -= latency;
                    if (position >= total && position - total < chunk.frames)
                    {
                        for (unsigned ch = 0; ch < fixture.channels; ++ch)
                        {
                            if (!(std::labs((long)pcm[(position - total) * fixture.channels + ch] -
                                            fixture.golden[p * fixture.channels + ch]) <= 2))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                }
                total += chunk.frames;
            }
            if (!(total == fixture.frames - latency))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* The original source is replaced before the native owner is queried and released. */
            Bytes empty;
            swf = movie(empty);
            if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_stream_sound_snapshot_info(source, &info) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_swf_stream_sound_snapshot_close(source);
        }
        for (unsigned bad = 0; bad < 2; ++bad)
        {
            Bytes swf = stream_movie(fixture, 0, bad == 0, bad == 1);
            if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_SWFStreamSoundSnapshot *source = (FVM_SWFStreamSoundSnapshot *)1;
            if (!(fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &source) == FVM_INVALID_ARGUMENT && !source))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
