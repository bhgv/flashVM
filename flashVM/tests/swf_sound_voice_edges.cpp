/* Native snapshot argument, empty/zero-play, unsupported-control and malformed-style contracts. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf_sound.h"
#include "swf_sound_fixture.hpp"

using namespace sound_fixture;


static Bytes body(unsigned flags)
{
    Bytes e;
    word(e, 7);
    e.push_back((unsigned char)flags);
    return e;
}


static void load(FVM_Context *c, const Bytes &e, unsigned codec = 12)
{
    const unsigned char raw[] = {0, 128, 255};
    Bytes defs;
    tag(defs, 14, sound(7, codec, 3, raw, 3));
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
    FVM_SWFSoundVoice *v = (FVM_SWFSoundVoice *)1;
    load(c, body(32));
    if (!(fvm_swf_sound_voice_open(c, 0, &v) == FVM_UNSUPPORTED && !v))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    load(c, body(0), 0x20);
    if (!(fvm_swf_sound_voice_open(c, 0, &v) == FVM_UNSUPPORTED && !v))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_open(c, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes e = body(1);
    integer(e, 3);
    load(c, e);
    if (!(fvm_swf_sound_voice_open(c, 0, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoiceInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_voice_info(v, &info) == FVM_OK && info.finished && !info.frames_per_play))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t n = 99;
    short pcm[4] = {42};
    if (!(fvm_swf_sound_voice_read(v, 1, pcm, 4, &n) == FVM_NOT_FOUND && !n && pcm[0] == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_close(v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e = body(4);
    word(e, 0);
    load(c, e);
    if (!(fvm_swf_sound_voice_open(c, 0, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_info(v, &info) == FVM_OK && info.finished && !info.play_count &&
          info.frames_per_play == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_close(v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned bad = 0; bad < 3; ++bad)
    {
        e = body(bad == 0 ? 1 : bad == 1 ? 2 : 8);
        if (bad < 2)
        {
            integer(e, bad == 0 ? 4 : 3);
        }
        else
        {
            e.push_back(2);
            for (unsigned i = 0; i < 2; ++i)
            {
                integer(e, 0);
                word(e, 32768);
                word(e, 32768);
            }
        }
        load(c, e);
        v = (FVM_SWFSoundVoice *)1;
        if (!(fvm_swf_sound_voice_open(c, 0, &v) == FVM_INVALID_ARGUMENT && !v))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Positive fixed-clock crop at 11025 Hz retains only the converted source interval. */
    e = body(3);
    integer(e, 4);
    integer(e, 8);
    load(c, e, 4);
    if (!(fvm_swf_sound_voice_open(c, 0, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_voice_info(v, &info) == FVM_OK && info.sample_rate == 11025 && info.source_first == 1 &&
          info.frames_per_play == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_read(v, 1, pcm, 4, &n) == FVM_OK && n == 1 && pcm[0] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_read(v, 2, pcm, 4, &n) == FVM_OK && n == 1 && pcm[0] == 32512))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_close(v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* No-multiple is copied metadata, not an output registration or synchronization operation. */
    load(c, body(16));
    if (!(fvm_swf_sound_voice_open(c, 0, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info.struct_size = 0;
    info.sound_id = 99;
    if (!(fvm_swf_sound_voice_info(v, &info) == FVM_INVALID_ARGUMENT && info.sound_id == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_voice_info(v, &info) == FVM_OK && info.flags == 16 && info.source_first == 0 &&
          info.frames_per_play == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_read(v, 1, pcm, 4, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_read(v, 1, 0, 4, &n) == FVM_INVALID_ARGUMENT && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_read(v, (size_t)-1, pcm, 4, &n) == FVM_INVALID_ARGUMENT && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_info(v, &info) == FVM_OK && !info.play_index && !info.frame_in_play))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_read(v, 2, pcm, 4, &n) == FVM_OK && n == 2 && pcm[0] == -32768 && pcm[1] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_info(v, &info) == FVM_OK && info.frame_in_play == 2 && !info.finished))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_close(v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    n = 99;
    if (!(fvm_swf_sound_voice_read(0, 1, pcm, 4, &n) == FVM_INVALID_ARGUMENT && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_info(0, &info) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
