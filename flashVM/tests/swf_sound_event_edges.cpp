/* Boundary styles, synchronization priority, empty/unsupported sounds and atomic failed output. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf_sound.h"
#include "swf_sound_fixture.hpp"

using namespace sound_fixture;


struct Sink
{
    unsigned writes, stops;
    FVM_Result result;
    short values[16];
    size_t count;
    FVM_Context *context;
    FVM_AudioChannel *channel;
};


static FVM_Result write(void *p, const FVM_MediaFrame *f)
{
    Sink *s = (Sink *)p;
    ++s->writes;
    if (!(f->size <= sizeof(s->values)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    s->count = f->size / 2;
    std::memcpy(s->values, f->data, f->size);
    if (!(fvm_swf_sound_event_submit(s->context, 0, s->channel) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return s->result;
}


static FVM_Result stop(void *p)
{
    ++((Sink *)p)->stops;
    return FVM_OK;
}


static Bytes event(unsigned id, unsigned flags)
{
    Bytes e;
    word(e, id);
    e.push_back((unsigned char)flags);
    return e;
}


static void load(FVM_Context *c, const Bytes &e, unsigned format = 0x0c, unsigned count = 3)
{
    Bytes defs;
    const unsigned char raw[] = {0, 128, 255};
    tag(defs, 14, sound(7, format, count, raw, (format == 0x0c) ? count : 3));
    tag(defs, 14, sound(8, 0x0c, 3, raw, 3));
    tag(defs, 15, e);
    Bytes swf = movie(defs);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
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
    Sink sink;
    std::memset(&sink, 0, sizeof(sink));
    sink.context = c;
    FVM_MediaOutput out = {sizeof(out), &sink, write, stop};
    if (!(fvm_audio_channel_create(c, &out, &sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Invalid source boundaries fail before output, without binding this channel. */
    for (unsigned bad = 0; bad < 3; ++bad)
    {
        Bytes e = event(7, bad == 0 ? 1 : bad == 1 ? 2 : 3);
        if (bad != 1)
        {
            integer(e, bad == 0 ? 4 : 3);
        }
        if (bad != 0)
        {
            integer(e, bad == 1 ? 3 : 1);
        }
        load(c, e);
        if (!(fvm_swf_sound_event_submit(c, 0, sink.channel) == FVM_INVALID_ARGUMENT && !sink.writes))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Metadata keeps unordered envelopes; render rejects non-increasing positions. */
    for (unsigned duplicate = 0; duplicate < 2; ++duplicate)
    {
        Bytes e = event(7, 8);
        e.push_back(2);
        integer(e, 1);
        word(e, 32768);
        word(e, 32768);
        integer(e, duplicate ? 1 : 0);
        word(e, 0);
        word(e, 0);
        load(c, e);
        FVM_SWFSoundEvent view;
        view.struct_size = sizeof(view);
        if (!(fvm_swf_sound_event_at(c, 0, &view) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_event_submit(c, 0, sink.channel) == FVM_INVALID_ARGUMENT && !sink.writes))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Zero points means unity gain; first/last points hold their gains outside the interval. */
    Bytes e = event(7, 8);
    e.push_back(0);
    load(c, e);
    sink.result = FVM_ERROR;
    if (!(fvm_swf_sound_event_submit(c, 0, sink.channel) == FVM_ERROR && sink.writes == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int active = 99;
    if (!(fvm_audio_channel_active(sink.channel, &active) == FVM_OK && !active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    sink.result = FVM_OK;
    e = event(8, 16);
    load(c, e);
    if (!(fvm_swf_sound_event_submit(c, 0, sink.channel) == FVM_OK && sink.writes == 2 && sink.values[0] == -32768 &&
          sink.values[1] == 0 && sink.values[2] == 32512))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_stop(sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_close(sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_create(c, &out, &sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e = event(7, 8);
    e.push_back(1);
    integer(e, 100);
    word(e, 8192);
    word(e, 24576);
    load(c, e);
    if (!(fvm_swf_sound_event_submit(c, 0, sink.channel) == FVM_OK && sink.values[0] == -16384 &&
          sink.values[2] == 16256))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_stop(sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* In/out use a 44100 Hz control clock even when the source runs at 11025 Hz. */
    if (!(fvm_audio_channel_close(sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_create(c, &out, &sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e = event(7, 3);
    integer(e, 4);
    integer(e, 8);
    load(c, e, 4);
    if (!(fvm_swf_sound_event_submit(c, 0, sink.channel) == FVM_OK && sink.count == 2 && sink.values[0] == 0 &&
          sink.values[1] == 32512))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_stop(sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Explicit zero loops produces no voice and cannot bind an otherwise unused channel. */
    if (!(fvm_audio_channel_close(sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_create(c, &out, &sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e = event(7, 4);
    word(e, 0);
    load(c, e);
    unsigned zero_writes = sink.writes;
    if (!(fvm_swf_sound_event_submit(c, 0, sink.channel) == FVM_OK && sink.writes == zero_writes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e = event(8, 0);
    load(c, e);
    if (!(fvm_swf_sound_event_submit(c, 0, sink.channel) == FVM_OK && sink.writes == zero_writes + 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_stop(sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Stop wins over unusable range/codec and does not bind an inactive unbound channel. */
    if (!(fvm_audio_channel_close(sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_create(c, &out, &sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e = event(7, 35);
    integer(e, 0xffffffffUL);
    integer(e, 0);
    load(c, e, 0x20);
    unsigned old = sink.writes;
    if (!(fvm_swf_sound_event_submit(c, 0, sink.channel) == FVM_OK && sink.writes == old))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e = event(7, 0);
    load(c, e, 0x20);
    if (!(fvm_swf_sound_event_submit(c, 0, sink.channel) == FVM_UNSUPPORTED && sink.writes == old))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    load(c, e, 0x0c, 0);
    if (!(fvm_swf_sound_event_submit(c, 0, sink.channel) == FVM_OK && sink.writes == old))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e = event(8, 0);
    load(c, e);
    if (!(fvm_swf_sound_event_submit(c, 0, sink.channel) == FVM_OK && sink.writes == old + 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_close(sink.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
