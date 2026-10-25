/* Snapshot storage is independent of play count; read/info/close remain allocation-free after VM teardown. */
#define main voice_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "flashvm/swf_sound.h"
#include "swf_sound_fixture.hpp"

using namespace sound_fixture;


static FVM_Result write(void *, const FVM_MediaFrame *)
{
    if (!(false))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_ERROR;
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
    const unsigned char raw[] = {0, 128, 255};
    Bytes defs;
    tag(defs, 14, sound(7, 12, 3, raw, 3));
    Bytes e;
    word(e, 7);
    e.push_back(4);
    word(e, 65535);
    tag(defs, 15, e);
    Bytes swf = movie(defs);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoice *v = 0;
    bool failed = false, passed = false;
    for (long boundary = 0; boundary < 16 && !passed; ++boundary)
    {
        v = (FVM_SWFSoundVoice *)1;
        allowance = boundary;
        FVM_Result r = fvm_swf_sound_voice_open(c, 0, &v);
        allowance = -1;
        if (r == FVM_OUT_OF_MEMORY)
        {
            failed = true;
            if (!(!v))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(r == FVM_OK && v))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            passed = true;
            if (!(fvm_swf_sound_voice_close(v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(failed && passed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = 5;
    v = (FVM_SWFSoundVoice *)1;
    if (!(fvm_swf_sound_voice_open(c, 0, &v) == FVM_OUT_OF_MEMORY && !v))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = 6;
    if (!(fvm_swf_sound_voice_open(c, 0, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_MediaOutput out = {sizeof(out), 0, write, stop};
    FVM_AudioChannel *channel = 0;
    if (!(fvm_audio_channel_create(c, &out, &channel) == FVM_OK &&
          fvm_swf_sound_event_submit(c, 0, channel) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    allowance = 0;
    short pcm[257];
    size_t total = 0, n = 0;
    for (;;)
    {
        FVM_Result r = fvm_swf_sound_voice_read(v, 257, pcm, 257, &n);
        if (r == FVM_NOT_FOUND)
        {
            if (!(!n))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            break;
        }
        if (!(r == FVM_OK && n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (size_t i = 0; i < n; ++i)
        {
            const short expected[] = {-32768, 0, 32512};
            if (!(pcm[i] == expected[(total + i) % 3]))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        total += n;
    }
    FVM_SWFSoundVoiceInfo info;
    info.struct_size = sizeof(info);
    if (!(total == 196605 && fvm_swf_sound_voice_info(v, &info) == FVM_OK && info.finished &&
          info.play_count == 65535 && info.play_index == 65535))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_close(v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    return 0;
}
