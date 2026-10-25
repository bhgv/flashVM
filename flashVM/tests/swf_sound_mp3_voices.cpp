/* MP3 intervals/loops/envelopes and converted mixing remain native after source VM teardown. */
#define main mp3_public_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mp3.cpp"

#undef main
#include "flashvm/swf_sound_mixer.h"
#include "../src/swf/sound_style.hpp"
#include "flashvm/gc.h"
#include <climits>


int main()
{
    unsigned long converted = 91;
    if (!(fvm_swf_sound_position(ULONG_MAX, 44100, &converted) == FVM_OK && converted == ULONG_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    converted = 91;
    if (!(fvm_swf_sound_position(ULONG_MAX, 48000, &converted) == FVM_INVALID_ARGUMENT && converted == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_position(1, 0, &converted) == FVM_INVALID_ARGUMENT && converted == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (size_t index = 0; index < sizeof(mp3_fixture::fixtures) / sizeof(mp3_fixture::fixtures[0]); ++index)
    {
        const mp3_fixture::Fixture &fixture = mp3_fixture::fixtures[index];
        FVM_Config cfg;
        FVM_Runtime *rt = 0;
        FVM_Context *c = 0;
        if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
              fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes payload, tags, control;
        word(payload, 529);
        for (size_t i = 0; i < fixture.size; ++i)
        {
            payload.push_back(fixture.data[i]);
        }
        tag(tags, 14, sound(7, 0x22, (unsigned long)fixture.frames - 529, payload.begin(), payload.size()));
        word(control, 7);
        control.push_back(15);
        integer(control, 441);
        integer(control, 881);
        word(control, 2);
        control.push_back(1);
        integer(control, 0);
        word(control, 8192);
        word(control, 8192);
        tag(tags, 15, control);
        Bytes swf = movie(tags);
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        size_t first = fixture.rate / 100;
        size_t length = (881UL * fixture.rate / 44100) + 1 - first;
        fvm_detail::vector<short> original(length * fixture.channels);
        size_t written = 0;
        if (!(fvm_swf_sound_read_pcm(c, 7, (unsigned long)first, length, original.begin(), original.size(), &written) ==
              FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundVoice *voice = 0;
        if (!(fvm_swf_sound_voice_open(c, 0, &voice) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundVoiceInfo info;
        info.struct_size = sizeof(info);
        if (!(fvm_swf_sound_voice_info(voice, &info) == FVM_OK && info.source_first == first &&
              info.frames_per_play == length && info.play_count == 2 && info.sample_rate == fixture.rate))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixerConfig geometry = {sizeof(geometry), 48000, 2, 3, FVM_SWF_SOUND_MIXER_CONVERT};
        FVM_SWFSoundMixer *mixer = 0;
        if (!(fvm_swf_sound_mixer_create_configured(&geometry, &mixer) == FVM_OK &&
              fvm_swf_sound_mixer_submit(mixer, c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
        for (size_t frame = 0; frame < length * 2; ++frame)
        {
            short pcm[2];
            if (!(fvm_swf_sound_voice_read(voice, 1, pcm, 2, &written) == FVM_OK && written == fixture.channels))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned ch = 0; ch < fixture.channels; ++ch)
            {
                if (!(pcm[ch] == (short)(original[(frame % length) * fixture.channels + ch] * 0.25)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        short pcm[64];
        if (!(fvm_swf_sound_mixer_read(mixer, 32, pcm, 64, &written) == FVM_OK && written == 64))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (size_t frame = 0; frame < 32; ++frame)
        {
            for (unsigned ch = 0; ch < 2; ++ch)
            {
                size_t at = frame * fixture.rate / 48000;
                long phase = (long)(frame * fixture.rate % 48000);
                unsigned source_channel = fixture.channels == 1 ? 0 : ch;
                long left = (short)(original[at * fixture.channels + source_channel] * 0.25);
                long right = (short)(original[(at + 1) * fixture.channels + source_channel] * 0.25);
                long numerator = left * (48000 - phase) + right * phase;
                long expected = numerator < 0 ? -(long)((unsigned long)(-numerator) / 48000) : numerator / 48000;
                if (!(pcm[frame * 2 + ch] == expected))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        fvm_swf_sound_voice_close(voice);
        fvm_swf_sound_mixer_close(mixer);
    }
    return 0;
}
