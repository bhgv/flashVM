/* Root stream header changes, variable PCM blocks, frame gaps and atomic malformed-input rejection. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf_stream_sound.h"
#include "swf_sound_fixture.hpp"

using namespace sound_fixture;


static Bytes header(unsigned flags, unsigned average = 1, unsigned playback = 6)
{
    Bytes h;
    h.push_back((unsigned char)playback);
    h.push_back((unsigned char)flags);
    word(h, average);
    return h;
}


static Bytes stream_movie(const Bytes &tags, unsigned version, unsigned frames)
{
    Bytes b = movie(tags, version);
    b[12] = (unsigned char)frames;
    b[13] = (unsigned char)(frames >> 8);
    return b;
}


static void load(FVM_Context *c, const Bytes &b)
{
    if (!(fvm_load_memory(c, b.begin(), b.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    size_t count = 91, n = 91;
    FVM_SWFStreamSoundInfo info;
    info.struct_size = sizeof(info);
    info.frame = 99;
    FVM_SWFStreamSoundBlock block;
    block.struct_size = sizeof(block);
    block.frame = 99;
    if (!(fvm_swf_stream_sound_count(c, &count) == FVM_UNSUPPORTED && count == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_count(0, &count) == FVM_INVALID_ARGUMENT && count == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_count(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes empty;
    load(c, movie(empty));
    if (!(fvm_swf_stream_sound_count(c, &count) == FVM_OK && !count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_at(c, 0, &info) == FVM_NOT_FOUND && info.frame == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 3; version <= 8; ++version)
    {
        for (unsigned stereo = 0; stereo < 2; ++stereo)
        {
            for (unsigned wide = 0; wide < 2; ++wide)
            {
                Bytes tags, pcm;
                tag(tags, 45, header((version >= 4 ? 0x30 : 0) | 12 | (wide ? 2 : 0) | stereo, 65535, 1));
                for (unsigned i = 0; i < 3 * (stereo + 1); ++i)
                {
                    if (wide)
                    {
                        if (version == 3)
                        {
                            short value = (short)(i % 2 ? 32767 : -32768);
                            unsigned char bytes[sizeof(short)];
                            std::memcpy(bytes, &value, sizeof(value));
                            for (size_t j = 0; j < sizeof(value); ++j)
                            {
                                pcm.push_back(bytes[j]);
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
                tag(tags, 1, empty);
                tag(tags, 45, header(4, 0));
                Bytes last;
                last.push_back(128);
                tag(tags, 19, last);
                load(c, stream_movie(tags, version, 4));
                if (!(fvm_swf_stream_sound_count(c, &count) == FVM_OK && count == 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_at(c, 0, &info) == FVM_OK && info.header_tag == 45 && info.frame == 1 &&
                      info.average_samples == 65535 && info.channels == stereo + 1 && info.sample_rate == 44100 &&
                      info.playback_sample_rate == 5512 && info.playback_channels == 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_block_count(c, 0, &count) == FVM_OK && count == 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_block_at(c, 0, 0, &block) == FVM_OK && block.frame == 1 &&
                      block.sample_count_known && block.sample_count == 3))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                short out[8];
                for (unsigned i = 0; i < 8; ++i)
                {
                    out[i] = 42;
                }
                if (!(fvm_swf_stream_sound_read_pcm(c, 0, 0, 1, 2, out, 8, &n) == FVM_OK && n == 2 * (stereo + 1)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (size_t i = 0; i < n; ++i)
                {
                    unsigned original = (stereo + 1) + (unsigned)i;
                    if (!(out[i] == (original % 2 ? (wide ? 32767 : 32512) : -32768)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_swf_stream_sound_read_pcm(c, 0, 0, 3, 0, 0, 0, &n) == FVM_OK && !n))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                out[0] = 42;
                if (!(fvm_swf_stream_sound_read_pcm(c, 0, 0, 3, 1, out, 8, &n) == FVM_INVALID_ARGUMENT && !n &&
                      out[0] == 42))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_read_pcm(c, 0, 0, 0, 3, out, 1, &n) == FVM_INVALID_ARGUMENT && !n &&
                      out[0] == 42))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_read_pcm(c, 0, 0, 0, 1, 0, 8, &n) == FVM_INVALID_ARGUMENT && !n))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_read_pcm(c, 0, 0, 0, 1, out, 8, 0) == FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_block_at(c, 0, 1, &block) == FVM_OK && block.frame == 3 &&
                      !block.sample_count && block.sample_count_known))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_read_pcm(c, 0, 1, 0, 0, 0, 0, &n) == FVM_OK && !n))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_block_count(c, 1, &count) == FVM_OK && count == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_at(c, 1, &info) == FVM_OK && info.frame == 4 && info.sample_rate == 11025 &&
                      !info.average_samples))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_stream_sound_read_pcm(c, 1, 0, 0, 1, out, 8, &n) == FVM_OK && n == 1 && out[0] == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                count = 99;
                if (!(fvm_swf_stream_sound_block_count(c, 2, &count) == FVM_NOT_FOUND && count == 99))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                block.frame = 99;
                if (!(fvm_swf_stream_sound_block_at(c, 0, 2, &block) == FVM_NOT_FOUND && block.frame == 99))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    /* Original head accepts ADPCM metadata, but decoding compressed stream blocks is explicitly unsupported. */
    for (unsigned version = 1; version <= 8; ++version)
    {
        Bytes tags;
        tag(tags, 18, header(0x1e));
        tag(tags, 19, empty);
        load(c, movie(tags, version));
        if (!(fvm_swf_stream_sound_at(c, 0, &info) == FVM_OK && info.format == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_sound_block_at(c, 0, 0, &block) == FVM_OK && !block.sample_count_known))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        short value = 42;
        if (!(fvm_swf_stream_sound_read_pcm(c, 0, 0, 0, 0, &value, 1, &n) == FVM_UNSUPPORTED && !n && value == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Signed MP3 latency is copied; stream data itself stays opaque. */
    {
        Bytes tags, h = header(0x2e);
        word(h, 65534);
        tag(tags, 18, h);
        load(c, movie(tags, 4));
        if (!(fvm_swf_stream_sound_at(c, 0, &info) == FVM_OK && info.latency_seek == -2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_sound_block_count(c, 0, &count) == FVM_OK && !count))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* A malformed later control rejects even an earlier selected valid block without output writes. */
    for (unsigned bad = 0; bad < 10; ++bad)
    {
        Bytes tags, p;
        p.push_back(128);
        tag(tags, 45, header(12));
        tag(tags, 19, p);
        tag(tags, 1, empty);
        if (bad == 0)
        {
            tag(tags, 19, p);
            tag(tags, 19, p);
        }
        else if (bad == 1)
        {
            Bytes h = header(14, 1, 0x86);
            tag(tags, 45, h);
        }
        else if (bad == 2)
        {
            Bytes h = header(14);
            h.push_back(0);
            tag(tags, 45, h);
        }
        else if (bad == 3)
        {
            tag(tags, 45, header(0x3f));
            tag(tags, 19, p);
        }
        else if (bad == 4)
        {
            tag(tags, 18, header(14));
        }
        else if (bad == 5)
        {
            tag(tags, 45, header(0x1c));
        }
        else if (bad == 6)
        {
            tag(tags, 45, header(0x2e));
        }
        else if (bad == 7)
        {
            Bytes h = header(0x22);
            word(h, 0);
            tag(tags, 45, h);
        }
        else if (bad == 8)
        {
            Bytes h;
            h.push_back(6);
            tag(tags, 45, h);
        }
        else
        {
            tag(tags, 1, empty);
            tag(tags, 45, header(12));
        }
        load(c, stream_movie(tags, 4, 2));
        count = 99;
        info.frame = 99;
        block.frame = 99;
        short out = 42;
        if (!(fvm_swf_stream_sound_count(c, &count) == FVM_INVALID_ARGUMENT && count == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_sound_at(c, 0, &info) == FVM_INVALID_ARGUMENT && info.frame == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_sound_block_at(c, 0, 0, &block) == FVM_INVALID_ARGUMENT && block.frame == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_sound_read_pcm(c, 0, 0, 0, 1, &out, 1, &n) == FVM_INVALID_ARGUMENT && !n && out == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    {
        Bytes tags;
        tag(tags, 19, empty);
        load(c, movie(tags));
        if (!(fvm_swf_stream_sound_count(c, &count) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    {
        Bytes tags;
        tag(tags, 45, header(12));
        load(c, movie(tags, 2));
        if (!(fvm_swf_stream_sound_count(c, &count) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    {
        Bytes tags;
        tag(tags, 45, header(0x3c));
        load(c, movie(tags, 3));
        if (!(fvm_swf_stream_sound_count(c, &count) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    info.struct_size = 0;
    block.struct_size = 0;
    if (!(fvm_swf_stream_sound_at(c, 0, &info) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_block_at(c, 0, 0, &block) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
