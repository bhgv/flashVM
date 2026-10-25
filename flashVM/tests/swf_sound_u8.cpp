/* Exhaustive unsigned PCM values, native/little-endian formats and host submission. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf_sound.h"
#include "swf_sound_fixture.hpp"

using namespace sound_fixture;


struct Sink
{
    short pcm[512];
    size_t count;
    unsigned calls;
};


static FVM_Result write(void *p, const FVM_MediaFrame *f)
{
    Sink *s = (Sink *)p;
    if (!(f->size <= sizeof(s->pcm)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::memcpy(s->pcm, f->data, f->size);
    s->count = f->size / sizeof(short);
    ++s->calls;
    return FVM_OK;
}


static FVM_Result stop(void *)
{
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Sink sink;
    std::memset(&sink, 0, sizeof(sink));
    FVM_MediaOutput out = {sizeof(out), &sink, write, stop};
    FVM_AudioChannel *channel = 0;
    if (!(fvm_audio_channel_create(c, &out, &channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 1; version <= 8; ++version)
    {
        for (unsigned format = 0; format <= 3; format += 3)
        {
            for (unsigned channels = 1; channels <= 2; ++channels)
            {
                if (format == 3 && version < 4)
                {
                    continue;
                }
                Bytes data;
                for (unsigned i = 0; i < 256; ++i)
                {
                    for (unsigned ch = 0; ch < channels; ++ch)
                    {
                        data.push_back((unsigned char)(ch ? 255 - i : i));
                    }
                }
                Bytes defs;
                tag(defs, 14, sound(5, (format << 4) | 12 | (channels - 1), 256, data.begin(), data.size()));
                Bytes swf = movie(defs, version);
                if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                short pcm[512];
                size_t n = 0;
                if (!(fvm_swf_sound_read_pcm(c, 5, 0, 256, pcm, 512, &n) == FVM_OK && n == data.size()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (size_t i = 0; i < n; ++i)
                {
                    if (!(pcm[i] == ((int)data[i] - 128) * 256))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_swf_sound_read_pcm(c, 5, 127, 2, pcm, 512, &n) == FVM_OK && n == 2 * channels &&
                      pcm[0] == -256))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_submit(c, 5, channel, 0, 256) == FVM_OK && sink.count == data.size()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (size_t i = 0; i < sink.count; ++i)
                {
                    if (!(sink.pcm[i] == ((int)data[i] - 128) * 256))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                pcm[0] = 999;
                if (!(fvm_swf_sound_read_pcm(c, 5, 255, 2, pcm, 512, &n) == FVM_INVALID_ARGUMENT && !n &&
                      pcm[0] == 999))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_read_pcm(c, 5, 0, 1, pcm, channels - 1, &n) == FVM_INVALID_ARGUMENT && !n &&
                      pcm[0] == 999))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_read_pcm(c, 5, 256, 0, 0, 0, &n) == FVM_OK && !n))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    if (!(sink.calls == 26))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_close(channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
