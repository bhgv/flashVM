/* Independent integer PCM values, frame gaps, empty blocks, seeking and atomic cursor failures. */
#define main stream_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_sound.cpp"

#undef main
#include "flashvm/swf_stream_pcm.h"


/* Compare source-derived channel values with bounded chunk reads across encoded block markers. */
int main()
{
    const short values[] = {-32768, -1234, 0, 1234, 32767};
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes empty;
    for (unsigned version = 3; version <= 8; ++version)
    {
        for (unsigned stereo = 0; stereo < 2; ++stereo)
        {
            for (unsigned wide = 0; wide < 2; ++wide)
            {
                Bytes tags, pcm;
                unsigned channels = stereo + 1;
                tag(tags, 45, header((version >= 4 ? 0x30 : 0) | 12 | (wide ? 2 : 0) | stereo));
                for (unsigned i = 0; i < 5 * channels; ++i)
                {
                    if (wide)
                    {
                        short v = values[i % 5];
                        if (version == 3)
                        {
                            unsigned char raw[sizeof(v)];
                            std::memcpy(raw, &v, sizeof(v));
                            for (size_t j = 0; j < sizeof(v); ++j)
                            {
                                pcm.push_back(raw[j]);
                            }
                        }
                        else
                        {
                            word(pcm, (unsigned short)v);
                        }
                    }
                    else
                    {
                        pcm.push_back((unsigned char)(i * 17));
                    }
                }
                tag(tags, 19, pcm);
                tag(tags, 1, empty);
                tag(tags, 1, empty);
                tag(tags, 19, empty);
                tag(tags, 1, empty);
                tag(tags, 19, pcm);
                load(c, stream_movie(tags, version, 4));
                FVM_SWFStreamSoundSnapshot *snapshot = 0;
                if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &snapshot) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_SWFStreamPCMReader r, independent;
                if (!(fvm_swf_stream_pcm_reader_init(&r, snapshot) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_pcm_reader_init(&independent, snapshot) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                load(c, movie(empty)); /* Original SWF storage no longer participates. */
                FVM_SWFStreamPCMChunk chunk;
                std::memset(&chunk, 0, sizeof(chunk));
                chunk.struct_size = sizeof(chunk);
                size_t written = 999;
                short out[4] = {42, 42, 42, 42};
                FVM_SWFStreamPCMReader saved = r;
                FVM_SWFStreamPCMChunk untouched = chunk;
                if (!(fvm_swf_stream_pcm_reader_next(&r, 0, out, 4, &chunk, &written) == FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_pcm_reader_next(&r, 2, out, channels - 1, &chunk, &written) ==
                      FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_pcm_reader_next(&r, 2, 0, 4, &chunk, &written) == FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!std::memcmp(&r, &saved, sizeof(r)) && !std::memcmp(&chunk, &untouched, sizeof(chunk)) &&
                      written == 999 && out[0] == 42))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (unsigned block = 0; block < 3; ++block)
                {
                    if (block == 1)
                    {
                        if (!(fvm_swf_stream_pcm_reader_next(&r, 2, 0, 0, &chunk, &written) == FVM_OK && !written))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(chunk.frame == 3 && chunk.block_index == 1 && !chunk.frames &&
                              chunk.flags == (FVM_STREAM_PCM_BEGIN | FVM_STREAM_PCM_END | FVM_STREAM_PCM_EMPTY)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        continue;
                    }
                    for (size_t first = 0; first < 5; first += 2)
                    {
                        size_t frames = first == 4 ? 1 : 2;
                        if (!(fvm_swf_stream_pcm_reader_next(&r, 2, out, 4, &chunk, &written) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(chunk.stream_index == 0 && chunk.block_index == block &&
                              chunk.frame == (block ? 4U : 1U)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(chunk.first == first && chunk.frames == frames && chunk.channels == channels &&
                              chunk.sample_rate == 44100 && written == frames * channels))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(chunk.flags ==
                              ((first == 0 ? FVM_STREAM_PCM_BEGIN : 0) | (first == 4 ? FVM_STREAM_PCM_END : 0))))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        for (size_t j = 0; j < written; ++j)
                        {
                            size_t i = first * channels + j;
                            short expected = wide ? values[i % 5] : (short)(((long)(i * 17) - 128) * 256);
                            if (!(out[j] == expected))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                }
                saved = r;
                untouched = chunk;
                written = 999;
                out[0] = 42;
                if (!(fvm_swf_stream_pcm_reader_next(&r, 2, out, 4, &chunk, &written) == FVM_NOT_FOUND))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!std::memcmp(&r, &saved, sizeof(r)) && !std::memcmp(&chunk, &untouched, sizeof(chunk)) &&
                      written == 999 && out[0] == 42))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_pcm_reader_seek(&r, 3, 1) == FVM_INVALID_ARGUMENT &&
                      !std::memcmp(&r, &saved, sizeof(r))))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_pcm_reader_seek(&r, 0, 6) == FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_pcm_reader_seek(&r, 0, 5) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_pcm_reader_next(&r, 1, 0, 0, &chunk, &written) == FVM_OK && !written &&
                      chunk.flags == FVM_STREAM_PCM_END))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_pcm_reader_seek(&r, 0, 3) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_pcm_reader_next(&r, (size_t)-1, out, 4, &chunk, &written) == FVM_OK &&
                      chunk.frames == 2 && written == 2 * channels))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_pcm_reader_seek(&r, 3, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_pcm_reader_next(&independent, 1, out, 4, &chunk, &written) == FVM_OK &&
                      written == channels && chunk.first == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_swf_stream_sound_snapshot_close(snapshot);
            }
        }
    }
    Bytes no_blocks;
    tag(no_blocks, 45, header(12));
    load(c, movie(no_blocks));
    FVM_SWFStreamSoundSnapshot *empty_snapshot = 0;
    if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &empty_snapshot) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamPCMReader empty_reader;
    if (!(fvm_swf_stream_pcm_reader_init(&empty_reader, empty_snapshot) == FVM_OK && !empty_reader.block_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamPCMChunk empty_chunk;
    empty_chunk.struct_size = sizeof(empty_chunk);
    size_t empty_written = 999;
    if (!(fvm_swf_stream_pcm_reader_next(&empty_reader, 1, 0, 0, &empty_chunk, &empty_written) == FVM_NOT_FOUND &&
          empty_written == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_pcm_reader_seek(&empty_reader, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(empty_snapshot);
    Bytes tags;
    tag(tags, 18, header(0x1e));
    load(c, movie(tags));
    FVM_SWFStreamSoundSnapshot *s = 0;
    if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &s) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamPCMReader r;
    std::memset(&r, 0xa5, sizeof(r));
    FVM_SWFStreamPCMReader saved = r;
    if (!(fvm_swf_stream_pcm_reader_init(&r, s) == FVM_UNSUPPORTED && !std::memcmp(&r, &saved, sizeof(r))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(s);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
