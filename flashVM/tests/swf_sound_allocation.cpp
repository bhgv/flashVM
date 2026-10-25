/* Lookup/read are allocation-free; native PCM submission OOM cannot activate or replay output. */
#define main sound_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "flashvm/swf_sound.h"
#include "swf_sound_fixture.hpp"

using namespace sound_fixture;
static unsigned writes, stops;


static FVM_Result write_pcm(void *, const FVM_MediaFrame *frame)
{
    if (!(frame->samples == 2 && frame->channels == 2 && frame->size == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++writes;
    return FVM_OK;
}


static FVM_Result stop_pcm(void *)
{
    ++stops;
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
    const unsigned char data[] = {1, 0, 255, 255, 2, 0, 254, 255};
    Bytes definitions;
    tag(definitions, 14, sound(7, 0x3f, 2, data, sizeof(data)));
    Bytes event;
    word(event, 7);
    event.push_back(8);
    event.push_back(1);
    integer(event, 44100);
    word(event, 32768);
    word(event, 0);
    tag(definitions, 15, event);
    Bytes swf = movie(definitions);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_MediaOutput output;
    output.struct_size = sizeof(output);
    output.userdata = 0;
    output.write = write_pcm;
    output.stop = stop_pcm;
    FVM_AudioChannel *channel = 0;
    if (!(fvm_audio_channel_create(c, &output, &channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short pcm[4] = {0};
    size_t count = 0;
    FVM_SWFSoundInfo info;
    info.struct_size = sizeof(info);
    allowance = 0;
    if (!(fvm_swf_sound_count(c, &count) == FVM_OK && count == 1 && fvm_swf_sound_at(c, 0, &info) == FVM_OK &&
          fvm_swf_sound_info(c, 7, &info) == FVM_OK && fvm_swf_sound_read_pcm(c, 7, 0, 2, pcm, 4, &count) == FVM_OK &&
          count == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundEvent ev;
    ev.struct_size = sizeof(ev);
    FVM_SWFSoundEnvelope point;
    point.struct_size = sizeof(point);
    if (!(fvm_swf_sound_event_count(c, &count) == FVM_OK && count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_at(c, 0, &ev) == FVM_OK && ev.sound_id == 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_envelope(c, 0, 0, &point) == FVM_OK && point.position_44 == 44100 &&
          point.left_level == 32768))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    bool failed = false, succeeded = false;
    for (long boundary = 0; boundary < 16 && !succeeded; ++boundary)
    {
        unsigned before = writes;
        allowance = boundary;
        FVM_Result result = fvm_swf_sound_submit(c, 7, channel, 0, 2);
        allowance = -1;
        if (result == FVM_OUT_OF_MEMORY)
        {
            failed = true;
            int active = 1;
            if (!(writes == before && fvm_audio_channel_active(channel, &active) == FVM_OK && !active))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(result == FVM_OK && writes == before + 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            succeeded = true;
        }
    }
    if (!(failed && succeeded))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long old = rt->config.memory_limit_bytes;
    rt->config.memory_limit_bytes = 1;
    if (!(fvm_swf_sound_submit(c, 7, channel, 0, 2) == FVM_OUT_OF_MEMORY && writes == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = old;
    allowance = 0;
    if (!(fvm_audio_stop_all(c) == FVM_OK && fvm_audio_channel_close(channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    if (!(stops == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
