/* Deadline allocation is transactional; registered query/adoption/retry/stop paths allocate nothing. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "swf_sound_output_fixture.hpp"

using namespace output_fixture;
using namespace conversion_fixture;


int main(int argc, char **argv)
{
    long begin = 0, end = 2;
    if (argc == 3)
    {
        if (!(!std::strcmp(argv[1], "--allocation-budget")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        char *tail = 0;
        begin = std::strtol(argv[2], &tail, 10);
        if (!(tail && !*tail && begin >= 0 && begin <= 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        end = begin + 1;
    }
    else
    {
        if (!(argc == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
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
    for (unsigned i = 0; i < 7; ++i)
    {
        samples.push_back(2000);
    }
    Bytes swf = movie(samples, 3, 1);
    FVM_SWFSoundVoice *source = 0, *control = 0;
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK && fvm_swf_sound_voice_open(c, 0, &source) == FVM_OK &&
          fvm_swf_sound_voice_open(c, 1, &control) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    for (long budget = begin; budget < end; ++budget)
    {
        FVM_SWFSoundVoice *voice = 0, *duplicate = 0;
        if (!(fvm_swf_sound_voice_clone(source, (size_t)-1, &voice) == FVM_OK &&
              fvm_swf_sound_voice_clone(control, (size_t)-1, &duplicate) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundMixer *mixer = create(48000, 2, 1);
        Device device;
        FVM_AudioQueueBackend table = backend(device);
        FVM_SWFSoundOutput *output = 0;
        if (!(fvm_swf_sound_output_create(mixer, &table, 16, 64, &output) == FVM_OK &&
              fvm_swf_sound_output_set_release(output, 8) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundOutputStorageInfo before, after;
        before.struct_size = sizeof(before);
        after.struct_size = sizeof(after);
        if (!(fvm_swf_sound_output_storage_info(output, &before) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        allowance = budget;
        FVM_Result result = fvm_swf_sound_output_set_identity_tracking(output, 1);
        allowance = -1;
        if (!budget)
        {
            if (!(result == FVM_OUT_OF_MEMORY && fvm_swf_sound_output_storage_info(output, &after) == FVM_OK &&
                  after.retained_bytes == before.retained_bytes && !after.identity_bytes))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_sound_output_set_identity_tracking(output, 1) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(result == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        allowance = 0;
        int playing = 9;
        size_t n = 0;
        if (!(fvm_swf_sound_output_submit_prepared_bounded(output, &voice, (size_t)-1) == FVM_OK && !voice))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        device.failure = 1;
        if (!(fvm_swf_sound_output_pump(output, &n) == FVM_ERROR && !n && !active(mixer)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_output_sound_active(output, 7, &playing) == FVM_OK && playing))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_output_submit_prepared_bounded(output, &duplicate, 0) == FVM_OK && !duplicate))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        device.failure = 0;
        if (!(fvm_swf_sound_output_pump(output, &n) == FVM_OK && n == 16))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_output_stop_id(output, 9) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_output_sound_active(output, 7, &playing) == FVM_OK && playing))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        device.failure = 4;
        if (!(fvm_swf_sound_output_stop_release(output) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        device.failure = 0;
        if (!(fvm_swf_sound_output_sound_active(output, 7, &playing) == FVM_OK && playing))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_output_stop_release(output) == FVM_OK &&
              fvm_swf_sound_output_sound_active(output, 7, &playing) == FVM_OK && !playing))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_output_set_identity_tracking(output, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_output_storage_info(output, &after) == FVM_OK &&
              after.retained_bytes == before.retained_bytes))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_swf_sound_output_close(output);
        fvm_swf_sound_mixer_close(mixer);
        if (!(allowance == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        allowance = -1;
    }
    fvm_swf_sound_voice_close(source);
    fvm_swf_sound_voice_close(control);
    return 0;
}
