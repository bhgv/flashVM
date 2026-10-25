/* Backend exceptions cannot consume an incoming owner or publish an identity query result. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_output_fixture.hpp"

using namespace output_fixture;
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
    samples.resize(7);
    Bytes swf = movie(samples, 3, 1);
    FVM_SWFSoundVoice *voice = 0;
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK && fvm_swf_sound_voice_open(c, 0, &voice) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    FVM_SWFSoundMixer *mixer = create(48000, 2, 1);
    Device device;
    FVM_AudioQueueBackend table = backend(device);
    FVM_SWFSoundOutput *output = 0;
    if (!(fvm_swf_sound_output_create(mixer, &table, 16, 64, &output) == FVM_OK &&
          fvm_swf_sound_output_set_identity_tracking(output, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (int thrown = 1; thrown <= 2; ++thrown)
    {
        device.thrown = thrown;
        FVM_Result expected = thrown == 1 ? FVM_OUT_OF_MEMORY : FVM_ERROR;
        FVM_SWFSoundVoice *saved = voice;
        int playing = 91;
        if (!(fvm_swf_sound_output_submit_prepared_bounded(output, &voice, (size_t)-1) == expected && voice == saved &&
              !active(mixer)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_output_sound_active(output, 7, &playing) == expected && playing == 91))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundOutputStorageInfo storage;
        storage.struct_size = sizeof(storage);
        if (!(fvm_swf_sound_output_storage_info(output, &storage) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_output_stop_id(output, 7) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        } /* No backend callback in this control. */
    }
    device.thrown = 0;
    if (!(fvm_swf_sound_output_submit_prepared_bounded(output, &voice, (size_t)-1) == FVM_OK && !voice))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t n = 0;
    if (!(fvm_swf_sound_output_pump(output, &n) == FVM_OK && n == 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop(output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_output_close(output);
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
