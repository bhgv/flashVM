/* Compile/use every public mixer operation through strict C89 declarations. */
#include "flashvm/swf_sound_mixer.h"
#include <assert.h>
int main(void)
{
    size_t additive_written = 91;
    short base = 123;
    FVM_SWFSoundMixer *mixer = 0;
    FVM_SWFSoundMixerInfo info;
    short output[4];
    size_t written = 99;
    if (fvm_swf_sound_mixer_add(0, 1, &base, 1, &additive_written) != FVM_INVALID_ARGUMENT ||
        additive_written || base != 123)
        return 1;
    assert(fvm_swf_sound_mixer_create(44100, 2, FVM_SWF_SOUND_MIXER_MAX_VOICES, &mixer) == FVM_OK);
    info.struct_size = sizeof(info);
    assert(fvm_swf_sound_mixer_info(mixer, &info) == FVM_OK && info.channels == 2 &&
           !info.active_voices);
    assert(fvm_swf_sound_mixer_submit(mixer, 0, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_swf_sound_mixer_read(mixer, 2, output, 4, &written) == FVM_OK && written == 4);
    assert(!output[0] && !output[1] && !output[2] && !output[3]);
    assert(fvm_swf_sound_mixer_stop_all(mixer) == FVM_OK);
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
