/* Native additive mixing and retirement allocate nothing after source VM teardown. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
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
    Samples source;
    for (unsigned i = 0; i < 600; ++i)
    {
        source.push_back(30000);
    }
    Bytes swf = movie(source, 3, 1);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixer *mixer = create(48000, 1, FVM_SWF_SOUND_MIXER_MAX_VOICES);
    for (unsigned i = 0; i < FVM_SWF_SOUND_MIXER_MAX_VOICES; ++i)
    {
        if (!(fvm_swf_sound_mixer_submit(mixer, c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    short pcm[700];
    for (unsigned i = 0; i < 700; ++i)
    {
        pcm[i] = -30000;
    }
    size_t written = 91;
    allowance = 0;
    if (!(fvm_swf_sound_mixer_add(mixer, 700, pcm, 700, &written) == FVM_OK && written == 700))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 700; ++i)
    {
        if (!(pcm[i] == (i < 654 ? 32767 : -30000)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_swf_sound_mixer_close(mixer);
    allowance = -1;
    return 0;
}
