/* Prepared native transfer/stop controls allocate nothing and preserve caller ownership on errors. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define main mp3_public_reference_main
#include "swf_sound_mp3.cpp"

#undef main
#include "flashvm/swf_sound_mixer.h"
#include "flashvm/gc.h"


int main()
{
    const mp3_fixture::Fixture &f = mp3_fixture::fixtures[1];
    Bytes payload, tags, event;
    word(payload, 529);
    for (size_t i = 0; i < f.size; ++i)
    {
        payload.push_back(f.data[i]);
    }
    tag(tags, 14, sound(7, 0x22, (unsigned long)f.frames - 529, payload.begin(), payload.size()));
    word(event, 7);
    event.push_back(0);
    tag(tags, 15, event);
    event[2] = FVM_SWF_SOUND_EVENT_NO_MULTIPLE;
    tag(tags, 15, event);
    event[2] = FVM_SWF_SOUND_EVENT_LOOPS;
    word(event, 0);
    tag(tags, 15, event);
    Bytes swf = movie(tags);
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoice *first = 0, *full = 0, *duplicate = 0, *partial = 0, *empty = 0;
    if (!(fvm_swf_sound_voice_open(c, 0, &first) == FVM_OK && fvm_swf_sound_voice_open(c, 0, &full) == FVM_OK &&
          fvm_swf_sound_voice_open(c, 1, &duplicate) == FVM_OK && fvm_swf_sound_voice_open(c, 0, &partial) == FVM_OK &&
          fvm_swf_sound_voice_open(c, 2, &empty) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short expected[64], scratch[64];
    size_t written = 0;
    if (!(fvm_swf_sound_read_pcm(c, 7, 0, 32, expected, 64, &written) == FVM_OK && written == 64))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_read(partial, 13, scratch, 64, &written) == FVM_OK && written == 26))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixer *mixer = 0, *wrong = 0, *release = 0;
    if (!(fvm_swf_sound_mixer_create(22050, 2, 1, &mixer) == FVM_OK &&
          fvm_swf_sound_mixer_create(44100, 1, 1, &wrong) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixerConfig geometry = {sizeof(geometry), 22050, 2, 1, FVM_SWF_SOUND_MIXER_DECLICK};
    if (!(fvm_swf_sound_mixer_create_configured(&geometry, &release) == FVM_OK))
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
    allowance = 0;
    FVM_SWFSoundVoice *saved = first;
    if (!(fvm_swf_sound_mixer_submit_prepared(0, &first) == FVM_INVALID_ARGUMENT && first == saved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit_prepared(wrong, &first) == FVM_UNSUPPORTED && first == saved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit_prepared(mixer, &first) == FVM_OK && !first))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    saved = full;
    if (!(fvm_swf_sound_mixer_submit_prepared(mixer, &full) == FVM_OUT_OF_MEMORY && full == saved))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoiceInfo progress;
    progress.struct_size = sizeof(progress);
    if (!(fvm_swf_sound_voice_info(full, &progress) == FVM_OK && !progress.frame_in_play))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit_prepared(mixer, &duplicate) == FVM_OK && !duplicate))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit_prepared(mixer, &empty) == FVM_OK && !empty))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_read(mixer, 2, scratch, 64, &written) == FVM_OK && written == 4 &&
          !std::memcmp(scratch, expected, 4 * sizeof(short))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_stop_id(mixer, 0) == FVM_INVALID_ARGUMENT &&
          fvm_swf_sound_mixer_stop_id(mixer, 65536) == FVM_INVALID_ARGUMENT &&
          fvm_swf_sound_mixer_stop_id(0, 7) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_stop_id(mixer, 9) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_read(mixer, 1, scratch, 64, &written) == FVM_OK && scratch[0] == expected[4] &&
          scratch[1] == expected[5]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_stop_id(mixer, 7) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixerInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mixer_info(mixer, &info) == FVM_OK && !info.active_voices))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit_prepared(mixer, &partial) == FVM_OK && !partial))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_read(mixer, 1, scratch, 64, &written) == FVM_OK && scratch[0] == expected[26] &&
          scratch[1] == expected[27]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_stop_id(mixer, 7) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit_prepared(release, &full) == FVM_OK && !full))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_stop_id(release, 7) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_read(release, 1, scratch, 64, &written) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_stop_id(release, 7) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    } /* Repeated stop never restarts release. */
    short tail[256];
    if (!(fvm_swf_sound_mixer_read(release, 128, tail, 256, &written) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (size_t i = 220; i < 256; ++i)
    {
        if (!(!tail[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_swf_sound_mixer_info(release, &info) == FVM_OK && !info.active_voices))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit_prepared(mixer, &first) == FVM_INVALID_ARGUMENT && !first))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit_prepared(mixer, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mixer_close(mixer);
    fvm_swf_sound_mixer_close(wrong);
    fvm_swf_sound_mixer_close(release);
    if (!(allowance == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    return 0;
}
