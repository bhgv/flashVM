/* Aggregate adoption is transactional and releases exact charges when native voices retire. */
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
    Samples samples;
    for (unsigned i = 0; i < 300; ++i)
    {
        samples.push_back(1200);
    }
    Bytes swf = movie(samples, 3, 1);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoice *a = 0, *b = 0, *duplicate = 0;
    if (!(fvm_swf_sound_voice_open(c, 0, &a) == FVM_OK && fvm_swf_sound_voice_open(c, 0, &b) == FVM_OK &&
          fvm_swf_sound_voice_open(c, 1, &duplicate) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixer *mixer = create(48000, 2, 2);
    FVM_SWFSoundMixerStorageInfo empty = {sizeof(empty), 0, 0, 0}, used = empty;
    FVM_SWFSoundVoiceStorageInfo voice = {sizeof(voice), 0, 0, 0};
    if (!(fvm_swf_sound_mixer_storage_info(mixer, &empty) == FVM_OK && !empty.voice_bytes &&
          empty.fixed_bytes == empty.retained_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixerConfig geometry = {sizeof(geometry), 48000, 2, 2, FVM_SWF_SOUND_MIXER_CONVERT};
    FVM_SWFSoundMixer *other = (FVM_SWFSoundMixer *)1;
    if (!(fvm_swf_sound_mixer_create_bounded(&geometry, empty.fixed_bytes - 1, &other) == FVM_OUT_OF_MEMORY && !other))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_create_bounded(&geometry, empty.fixed_bytes, &other) == FVM_OK && other))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_storage_info(other, &used) == FVM_OK && used.fixed_bytes == empty.fixed_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mixer_close(other);
    if (!(fvm_swf_sound_voice_storage_info(a, &voice) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoice *tail = 0, *again = 0;
    if (!(fvm_swf_sound_voice_open(c, 0, &tail) == FVM_OK && fvm_swf_sound_voice_open(c, 1, &again) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixer *release = 0;
    geometry.flags |= FVM_SWF_SOUND_MIXER_DECLICK;
    if (!(fvm_swf_sound_mixer_create_configured(&geometry, &release) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    allowance = 0;
    size_t budget = empty.retained_bytes + voice.retained_bytes;
    int active = 99;
    if (!(fvm_swf_sound_mixer_sound_active(mixer, 7, &active) == FVM_OK && !active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_sound_active(mixer, 0, &active) == FVM_INVALID_ARGUMENT && !active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoice *saved = a;
    if (!(fvm_swf_sound_mixer_submit_prepared_bounded(mixer, &a, budget - 1) == FVM_OUT_OF_MEMORY && a == saved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit_prepared_bounded(mixer, &a, budget) == FVM_OK && !a))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_storage_info(mixer, &used) == FVM_OK && used.retained_bytes == budget &&
          used.voice_bytes == voice.retained_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_sound_active(mixer, 7, &active) == FVM_OK && active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    saved = b;
    if (!(fvm_swf_sound_mixer_submit_prepared_bounded(mixer, &b, budget) == FVM_OUT_OF_MEMORY && b == saved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit_prepared_bounded(mixer, &duplicate, 0) == FVM_OK && !duplicate))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short pcm[1024];
    size_t n = 0;
    if (!(fvm_swf_sound_mixer_read(mixer, 512, pcm, 1024, &n) == FVM_OK && n == 1024))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_storage_info(mixer, &used) == FVM_OK && used.retained_bytes == empty.retained_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_sound_active(mixer, 7, &active) == FVM_OK && !active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit_prepared_bounded(mixer, &b, budget) == FVM_OK && !b))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_stop_all(mixer) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    used.struct_size = 0;
    if (!(fvm_swf_sound_mixer_storage_info(mixer, &used) == FVM_INVALID_ARGUMENT && !used.struct_size))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit_prepared(release, &tail) == FVM_OK && !tail))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    used.struct_size = sizeof(used);
    if (!(fvm_swf_sound_mixer_storage_info(release, &used) == FVM_OK && used.voice_bytes == voice.retained_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t releasing_bytes = used.retained_bytes;
    if (!(fvm_swf_sound_mixer_stop_id(release, 7) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_sound_active(release, 7, &active) == FVM_OK && !active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_storage_info(release, &used) == FVM_OK && used.retained_bytes == releasing_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit_prepared_bounded(release, &again, releasing_bytes + voice.retained_bytes) ==
              FVM_OK &&
          !again))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_sound_active(release, 7, &active) == FVM_OK && active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_read(release, 512, pcm, 1024, &n) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_storage_info(release, &used) == FVM_OK && !used.voice_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mixer_close(release);
    fvm_swf_sound_mixer_close(mixer);
    if (!(allowance == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    return 0;
}
