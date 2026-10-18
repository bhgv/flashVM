/* Strict C89 configured creation/query, without changing the legacy info struct ABI. */
#include "flashvm/swf_sound_mixer.h"
#include <assert.h>
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
    assert(fvm_swf_sound_mixer_create_configured(&config, &mixer) == FVM_OK);
    config.flags = 0;
    config.sample_rate = 44100;
    assert(fvm_swf_sound_mixer_get_config(mixer, &config) == FVM_OK &&
           config.sample_rate == 48000 && config.flags == FVM_SWF_SOUND_MIXER_CONVERT);
    info.struct_size = sizeof(info);
    assert(fvm_swf_sound_mixer_info(mixer, &info) == FVM_OK && info.sample_rate == 48000 &&
           !info.active_voices);
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
