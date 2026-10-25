/* Actual StartSound PCM callbacks, styles, dedicated resource identity and reentrant reload. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf_sound.h"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include "flashvm/execution_mode.h"
#include "swf_sound_fixture.hpp"

using namespace sound_fixture;


struct Sink
{
    short values[64];
    size_t count;
    unsigned writes, stops;
    FVM_Result write_result, stop_result;
    FVM_Context *context;
    const Bytes *reload;
    bool reenter;
};


static FVM_Result write(void *p, const FVM_MediaFrame *frame)
{
    Sink *s = (Sink *)p;
    ++s->writes;
    if (!(frame->size <= sizeof(s->values) && frame->seconds == 0 && frame->has_timestamp))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (s->reenter)
    {
        if (!(fvm_swf_sound_event_submit(s->context, 0, 0) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(s->context, 0) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (s->reload)
    {
        if (!(fvm_load_memory(s->context, s->reload->begin(), s->reload->size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    std::memcpy(s->values, frame->data, frame->size);
    s->count = frame->size / sizeof(short);
    return s->write_result;
}


static FVM_Result stop(void *p)
{
    Sink *s = (Sink *)p;
    ++s->stops;
    return s->stop_result;
}


static Bytes event(unsigned id, unsigned flags)
{
    Bytes b;
    word(b, id);
    b.push_back((unsigned char)flags);
    if (flags & 1)
    {
        integer(b, 1);
    }
    if (flags & 2)
    {
        integer(b, 3);
    }
    if (flags & 4)
    {
        word(b, 2);
    }
    if (flags & 8)
    {
        b.push_back(2);
        integer(b, 0);
        word(b, 0);
        word(b, 32768);
        integer(b, 2);
        word(b, 32768);
        word(b, 0);
    }
    return b;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 1; version <= 8; ++version)
    {
        for (unsigned stereo = 0; stereo < 2; ++stereo)
        {
            for (unsigned mode = 0; mode < 2; ++mode)
            {
                FVM_Context *c = 0, *other = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_execution_set_mode(c, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Bytes data;
                const short source[] = {1000, -2000, 3000, -4000, 5000};
                for (unsigned i = 0; i < 5; ++i)
                {
                    for (unsigned ch = 0; ch <= stereo; ++ch)
                    {
                        short v = source[i];
                        if (version < 4)
                        {
                            unsigned char raw[2];
                            std::memcpy(raw, &v, 2);
                            data.push_back(raw[0]);
                            data.push_back(raw[1]);
                        }
                        else
                        {
                            word(data, (unsigned short)v);
                        }
                    }
                }
                Bytes defs;
                tag(defs, 14, sound(7, (version < 4 ? 0 : 0x30) | 14 | stereo, 5, data.begin(), data.size()));
                tag(defs, 14, sound(8, (version < 4 ? 0 : 0x30) | 14 | stereo, 5, data.begin(), data.size()));
                tag(defs, 15, event(7, 15));
                tag(defs, 15, event(7, 16));
                tag(defs, 15, event(7, 0));
                tag(defs, 15, event(7, 32));
                tag(defs, 15, event(8, 0));
                Bytes zero = event(7, 4);
                zero[3] = zero[4] = 0;
                tag(defs, 15, zero);
                Bytes swf = movie(defs, version);
                if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Sink sink;
                std::memset(&sink, 0, sizeof(sink));
                sink.context = c;
                sink.reenter = true;
                FVM_MediaOutput out = {sizeof(out), &sink, write, stop};
                FVM_AudioChannel *channel = 0, *foreign = 0;
                if (!(fvm_audio_channel_create(c, &out, &channel) == FVM_OK &&
                      fvm_audio_channel_create(other, &out, &foreign) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_event_submit(c, 0, foreign) == FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_event_submit(c, 0, channel) == FVM_OK && sink.writes == 1 &&
                      sink.count == 6 * (stereo + 1)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!stereo)
                {
                    for (unsigned i = 0; i < 6; ++i)
                    {
                        if (!(sink.values[i] == source[1 + i % 3] / 2))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                else
                {
                    const short expected[] = {0, -2000, 1500, 1500, -4000, 0, -2000, 0, 3000, 0, -4000, 0};
                    if (!(!std::memcmp(expected, sink.values, sizeof(expected))))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_swf_sound_event_submit(c, 1, channel) == FVM_OK && sink.writes == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_event_submit(c, 2, channel) == FVM_INVALID_ARGUMENT && sink.writes == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_event_submit(c, 4, channel) == FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_submit(c, 8, channel, 0, 1) == FVM_INVALID_ARGUMENT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                sink.stop_result = FVM_ERROR;
                if (!(fvm_swf_sound_event_submit(c, 3, channel) == FVM_ERROR))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                int active = 0;
                if (!(fvm_audio_channel_active(channel, &active) == FVM_OK && active))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                sink.stop_result = FVM_OK;
                if (!(fvm_swf_sound_event_submit(c, 3, channel) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_audio_channel_active(channel, &active) == FVM_OK && !active))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_event_submit(c, 5, channel) == FVM_OK && sink.writes == 1 &&
                      sink.count == 6 * (stereo + 1)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const unsigned char halt[] = {9, 0};
                if (!(fvm_avm1_execute(c, halt, sizeof(halt)) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Bytes empty, replacement = movie(empty, version);
                sink.reload = &replacement;
                if (!(fvm_swf_sound_event_submit(c, 0, channel) == FVM_OK && sink.count == 6 * (stereo + 1)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_event_submit(c, 0, channel) == FVM_NOT_FOUND))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_audio_channel_close(channel) == FVM_OK && fvm_audio_channel_close(foreign) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(other);
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
