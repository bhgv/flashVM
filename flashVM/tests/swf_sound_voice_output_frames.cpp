/* Native duration equals actual converted mixer retirement from every tested source cursor. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_conversion_fixture.hpp"

using namespace conversion_fixture;


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
    const unsigned outputs[] = {5512, 8000, 11025, 22050, 32000, 44100, 48000, 192000};
    Samples samples;
    for (unsigned i = 0; i < 7; ++i)
    {
        samples.push_back((short)(1000 + i * 100));
    }
    for (unsigned input = 0; input < 4; ++input)
    {
        Bytes swf = movie(samples, input, 1, 3);
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned output = 0; output < 8; ++output)
        {
            for (unsigned position = 0; position <= 21; ++position)
            {
                FVM_SWFSoundVoice *voice = 0;
                if (!(fvm_swf_sound_voice_open(c, 0, &voice) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_voice_seek(voice, position / 7, position % 7) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                double count = -1;
                if (!(fvm_swf_sound_voice_output_frames(voice, outputs[output], &count) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(count == std::ceil((21 - position) * (double)outputs[output] / rates[input])))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_SWFSoundMixer *mixer = create(outputs[output], 1, 1);
                if (!(fvm_swf_sound_mixer_submit_prepared(mixer, &voice) == FVM_OK && !voice))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                size_t actual = 0, n = 0;
                short value = 0;
                while (active(mixer))
                {
                    if (!(fvm_swf_sound_mixer_read(mixer, 1, &value, 1, &n) == FVM_OK && n == 1))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    ++actual;
                }
                if (!((double)actual == count))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_swf_sound_mixer_close(mixer);
            }
        }
    }
    FVM_SWFSoundVoice *voice = 0;
    if (!(fvm_swf_sound_voice_open(c, 0, &voice) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    double unchanged = 123;
    if (!(fvm_swf_sound_voice_output_frames(voice, 0, &unchanged) == FVM_INVALID_ARGUMENT && unchanged == 123))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_output_frames(0, 48000, &unchanged) == FVM_INVALID_ARGUMENT && unchanged == 123))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_output_frames(voice, 48000, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_output_frames(voice, 48000, &unchanged) == FVM_OK && unchanged == 23))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_voice_close(voice);
    return 0;
}
