/* Render allocation failures leave native output/binding untouched; limits cover both PCM buffers. */
#define main event_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "flashvm/swf_sound.h"
#include "swf_sound_fixture.hpp"

using namespace sound_fixture;
static unsigned writes;


static FVM_Result write(void *, const FVM_MediaFrame *f)
{
    ++writes;
    if (!(f->samples == 4 && f->size == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned char data[] = {232, 3, 24, 252};
    Bytes defs;
    tag(defs, 14, sound(7, 0x3e, 2, data, 4));
    Bytes e;
    word(e, 7);
    e.push_back(4);
    word(e, 2);
    tag(defs, 15, e);
    Bytes query;
    word(query, 7);
    query.push_back(16);
    tag(defs, 15, query);
    query[2] = 32;
    tag(defs, 15, query);
    Bytes swf = movie(defs);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_MediaOutput out = {sizeof(out), 0, write, stop};
    FVM_AudioChannel *channel = 0;
    if (!(fvm_audio_channel_create(c, &out, &channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool failed = false, passed = false;
    for (long boundary = 0; boundary < 16 && !passed; ++boundary)
    {
        allowance = boundary;
        FVM_Result r = fvm_swf_sound_event_submit(c, 0, channel);
        allowance = -1;
        if (r == FVM_OUT_OF_MEMORY)
        {
            failed = true;
            int active = 99;
            if (!(!writes && fvm_audio_channel_active(channel, &active) == FVM_OK && !active))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(r == FVM_OK && writes == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            passed = true;
        }
    }
    if (!(failed && passed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = 0;
    if (!(fvm_swf_sound_event_submit(c, 1, channel) == FVM_OK && writes == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_submit(c, 2, channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    if (!(fvm_audio_channel_stop(channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long limit = rt->config.memory_limit_bytes;
    rt->config.memory_limit_bytes = 11;
    if (!(fvm_swf_sound_event_submit(c, 0, channel) == FVM_OUT_OF_MEMORY && writes == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = 12;
    if (!(fvm_swf_sound_event_submit(c, 0, channel) == FVM_OK && writes == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = limit;
    allowance = 0;
    if (!(fvm_audio_channel_stop(channel) == FVM_OK && fvm_audio_channel_close(channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
