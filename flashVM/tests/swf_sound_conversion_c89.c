/* Strict C89 configured creation/query, without changing the legacy info struct ABI. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf_sound_mixer.h"


int main(void)
{
    FVM_SWFSoundMixerConfig config;
    FVM_SWFSoundMixerInfo info;
    FVM_SWFSoundMixer *mixer = 0;
    config.struct_size = sizeof(config);
    config.sample_rate = 48000;
    config.channels = 2;
    config.voice_capacity = 4;
    config.flags = FVM_SWF_SOUND_MIXER_CONVERT;
    if (!(fvm_swf_sound_mixer_create_configured(&config, &mixer) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    config.flags = 0;
    config.sample_rate = 44100;
    if (!(fvm_swf_sound_mixer_get_config(mixer, &config) == FVM_OK && config.sample_rate == 48000 &&
          config.flags == FVM_SWF_SOUND_MIXER_CONVERT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mixer_info(mixer, &info) == FVM_OK && info.sample_rate == 48000 && !info.active_voices))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
