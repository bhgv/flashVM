/* Compile/use every public mixer operation through strict C89 declarations. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf_sound_mixer.h"


int main(void)
{
    size_t additive_written = 91;
    short base = 123;
    FVM_SWFSoundMixer *mixer = 0;
    FVM_SWFSoundMixerInfo info;
    short output[4];
    size_t written = 99;
    if (fvm_swf_sound_mixer_add(0, 1, &base, 1, &additive_written) != FVM_INVALID_ARGUMENT || additive_written ||
        base != 123)
    {
        return 1;
    }
    if (!(fvm_swf_sound_mixer_create(44100, 2, FVM_SWF_SOUND_MIXER_MAX_VOICES, &mixer) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mixer_info(mixer, &info) == FVM_OK && info.channels == 2 && !info.active_voices))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit(mixer, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_read(mixer, 2, output, 4, &written) == FVM_OK && written == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!output[0] && !output[1] && !output[2] && !output[3]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_stop_all(mixer) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
