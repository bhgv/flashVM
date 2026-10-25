/* Context-wide bound-voice no-multiple, stop-all-by-ID, first errors and callback reload. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf_sound.h"
#include "flashvm/gc.h"
#include "swf_sound_fixture.hpp"

using namespace sound_fixture;


struct Sink
{
    FVM_Context *context;
    FVM_AudioChannel *channel;
    unsigned writes, stops;
    FVM_Result error;
    const Bytes *reload;
};


static FVM_Result write(void *p, const FVM_MediaFrame *frame)
{
    Sink *s = (Sink *)p;
    ++s->writes;
    if (!(frame->samples == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_OK;
}


static FVM_Result stop(void *p)
{
    Sink *s = (Sink *)p;
    ++s->stops;
    if (!(fvm_swf_sound_event_submit(s->context, 0, s->channel) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_close(s->channel) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(s->context, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (s->reload)
    {
        if (!(fvm_load_memory(s->context, s->reload->begin(), s->reload->size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return s->error;
}


static Bytes fixture()
{
    const unsigned char pcm[] = {0, 255};
    Bytes defs;
    tag(defs, 14, sound(7, 12, 2, pcm, 2));
    tag(defs, 14, sound(8, 12, 2, pcm, 2));
    for (unsigned i = 0; i < 5; ++i)
    {
        Bytes e;
        word(e, i == 3 ? 8 : 7);
        e.push_back(i == 1 ? 16 : i == 2 ? 32 : 0);
        tag(defs, 15, e);
    }
    return movie(defs);
}


static void inactive(FVM_AudioChannel *c, int expected)
{
    int active = 99;
    if (!(fvm_audio_channel_active(c, &active) == FVM_OK && active == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes swf = fixture();
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK &&
          fvm_load_memory(other, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Sink sinks[5];
    std::memset(sinks, 0, sizeof(sinks));
    for (unsigned i = 0; i < 5; ++i)
    {
        sinks[i].context = i == 4 ? other : c;
        FVM_MediaOutput out = {sizeof(out), &sinks[i], write, stop};
        if (!(fvm_audio_channel_create(sinks[i].context, &out, &sinks[i].channel) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Two overlap instances, another resource and an independent context can play together. */
    if (!(fvm_swf_sound_event_submit(c, 0, sinks[0].channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_submit(c, 0, sinks[1].channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_submit(c, 3, sinks[2].channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_submit(other, 0, sinks[4].channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_submit(c, 1, sinks[3].channel) == FVM_OK && !sinks[3].writes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inactive(sinks[3].channel, 0);
    /* The unbound suppressed channel stays usable for a different resource. */
    if (!(fvm_swf_sound_event_submit(c, 3, sinks[3].channel) == FVM_OK && sinks[3].writes == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_stop(sinks[3].channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_close(sinks[3].channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_MediaOutput out = {sizeof(out), &sinks[3], write, stop};
    if (!(fvm_audio_channel_create(c, &out, &sinks[3].channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* A first failing stop cannot prevent the later voice from being stopped. */
    sinks[0].error = FVM_ERROR;
    sinks[1].error = FVM_OUT_OF_MEMORY;
    if (!(fvm_swf_sound_event_submit(c, 2, sinks[3].channel) == FVM_ERROR && sinks[0].stops == 1 &&
          sinks[1].stops == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inactive(sinks[0].channel, 1);
    inactive(sinks[1].channel, 1);
    inactive(sinks[2].channel, 1);
    inactive(sinks[4].channel, 1);
    if (!(sinks[2].stops == 0 && sinks[4].stops == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    sinks[1].error = FVM_OK;
    if (!(fvm_swf_sound_event_submit(c, 2, sinks[3].channel) == FVM_ERROR && sinks[0].stops == 2 &&
          sinks[1].stops == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inactive(sinks[1].channel, 0);
    if (!(fvm_swf_sound_event_submit(c, 1, sinks[3].channel) == FVM_OK && sinks[3].writes == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Reload during the first callback cannot invalidate matching registrations. */
    if (!(fvm_swf_sound_event_submit(c, 0, sinks[1].channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes empty, replacement = movie(empty);
    sinks[0].error = FVM_OK;
    sinks[0].reload = &replacement;
    if (!(fvm_swf_sound_event_submit(c, 2, sinks[3].channel) == FVM_OK && sinks[0].stops == 3 && sinks[1].stops == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inactive(sinks[0].channel, 0);
    inactive(sinks[2].channel, 1);
    inactive(sinks[4].channel, 1);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_submit(c, 1, sinks[3].channel) == FVM_OK && sinks[3].writes == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_submit(c, 2, sinks[3].channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inactive(sinks[3].channel, 0);
    for (unsigned i = 0; i < 5; ++i)
    {
        sinks[i].reload = 0;
        if (!(fvm_audio_channel_close(sinks[i].channel) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
