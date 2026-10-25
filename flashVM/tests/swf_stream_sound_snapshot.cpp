/* Independent indexed PCM snapshots: geometry, lifecycle and atomic errors. */
#define main stream_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_sound.cpp"

#undef main


int main()
{
    if (!(stream_reference_main() == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
                tag(tags, 45, header((version >= 4 ? 0x30 : 0) | 12 | (wide ? 2 : 0) | stereo));
                for (unsigned i = 0; i < 4 * (stereo + 1); ++i)
                {
                    if (wide)
                    {
                        if (version == 3)
                        {
                            short v = (short)(i % 2 ? 32767 : -32768);
                            unsigned char raw[sizeof(v)];
                            std::memcpy(raw, &v, sizeof(v));
                            for (size_t j = 0; j < sizeof(v); ++j)
                            {
                                pcm.push_back(raw[j]);
                            }
                        }
                        else
                        {
                            word(pcm, i % 2 ? 32767 : 32768);
                        }
                    }
                    else
                    {
                        pcm.push_back(i % 2 ? 255 : 0);
                    }
                }
                tag(tags, 19, pcm);
                tag(tags, 1, empty);
                tag(tags, 1, empty);
                tag(tags, 19, empty);
                load(c, stream_movie(tags, version, 3));
                FVM_SWFStreamSoundSnapshot *s = 0;
                if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &s) == FVM_OK && s))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_SWFStreamSoundInfo info;
                info.struct_size = sizeof(info);
                if (!(fvm_swf_stream_sound_snapshot_info(s, &info) == FVM_OK && info.channels == stereo + 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                size_t count = 99, n = 99, m = 99;
                if (!(fvm_swf_stream_sound_snapshot_block_count(s, &count) == FVM_OK && count == 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                short a[8], b[8];
                if (!(fvm_swf_stream_sound_read_pcm(c, 0, 0, 1, 3, a, 8, &n) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                load(c, movie(empty));
                if (!(fvm_swf_stream_sound_snapshot_read_pcm(s, 0, 1, 3, b, 8, &m) == FVM_OK && m == n &&
                      !std::memcmp(a, b, n * sizeof(short))))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_SWFStreamSoundBlock block;
                block.struct_size = sizeof(block);
                if (!(fvm_swf_stream_sound_snapshot_block_at(s, 0, &block) == FVM_OK && block.data_size == pcm.size() &&
                      !std::memcmp(block.data, pcm.begin(), pcm.size())))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_snapshot_block_at(s, 1, &block) == FVM_OK && block.frame == 3 && block.data))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_snapshot_read_pcm(s, 1, 0, 0, 0, 0, &m) == FVM_OK && !m))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                b[0] = 42;
                if (!(fvm_swf_stream_sound_snapshot_read_pcm(s, 0, 4, 1, b, 8, &m) == FVM_INVALID_ARGUMENT && !m &&
                      b[0] == 42))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                block.frame = 99;
                if (!(fvm_swf_stream_sound_snapshot_block_at(s, 2, &block) == FVM_NOT_FOUND && block.frame == 99))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_snapshot_info(s, 0) == FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_swf_stream_sound_snapshot_close(s);
            }
        }
    }
    {
        Bytes tags;
        tag(tags, 45, header(12));
        load(c, movie(tags));
        FVM_SWFStreamSoundSnapshot *s = 0;
        if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &s) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        size_t count = 99;
        if (!(fvm_swf_stream_sound_snapshot_block_count(s, &count) == FVM_OK && !count))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_swf_stream_sound_snapshot_close(s);
        if (!(fvm_swf_stream_sound_snapshot_open(c, 1, &s) == FVM_NOT_FOUND && !s))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tags.clear();
        tag(tags, 18, header(0x1e));
        load(c, movie(tags));
        if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &s) == FVM_OK && s))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_swf_stream_sound_snapshot_close(s);
        tags.clear();
        tag(tags, 45, header(12));
        tag(tags, 19, empty);
        tag(tags, 19, empty);
        load(c, movie(tags));
        if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &s) == FVM_INVALID_ARGUMENT && !s))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Bytes tags, p;
    p.push_back(128);
    tag(tags, 45, header(12));
    tag(tags, 19, empty);
    load(c, movie(tags));
    FVM_SWFStreamSoundSnapshot *s = 0;
    if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &s) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    size_t n = 99;
    if (!(fvm_swf_stream_sound_snapshot_read_pcm(s, 0, 0, 0, 0, 0, &n) == FVM_OK && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(s);
    fvm_swf_stream_sound_snapshot_close(0);
    if (!(fvm_swf_stream_sound_snapshot_open(0, 0, &s) == FVM_INVALID_ARGUMENT && !s))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_read_pcm(0, 0, 0, 0, 0, 0, &n) == FVM_INVALID_ARGUMENT && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
