/* Buffered last samples remain active for SyncNoMultiple and are discarded by immediate stop. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_conversion_fixture.hpp"
#include "flashvm/gc.h"

using namespace conversion_fixture;


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Samples samples;
    samples.push_back(-12345);
    Bytes swf = movie(samples, 3, 1);
    if (!(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixer *mixer = create(192000, 2, 1);
    if (!(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short out[16];
    size_t written = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        if (!(fvm_swf_sound_mixer_read(mixer, 1, out, 2, &written) == FVM_OK && written == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(out[0] == -12345 && out[1] == -12345 && active(mixer) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mixer_submit(mixer, context, 1) == FVM_OK && active(mixer) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OUT_OF_MEMORY && active(mixer) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit(mixer, context, 2) == FVM_OK && !active(mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_read(mixer, 1, out, 2, &written) == FVM_OK && !out[0] && !out[1]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    /* Slot reuse resets phase and buffered frames; source VM is no longer needed. */
    if (!(fvm_swf_sound_mixer_read(mixer, 8, out, 16, &written) == FVM_OK && written == 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 8; ++i)
    {
        if (!(out[i * 2] == (i < 5 ? -12345 : 0) && out[i * 2 + 1] == out[i * 2]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!active(mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
