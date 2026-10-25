/* Optional fixed deadlines are transactional, independently charged, and query no source VM. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_output_fixture.hpp"

using namespace output_fixture;


int main()
{
    FVM_SWFSoundMixer *mixer = conversion_fixture::create(48000, 2);
    Device device;
    FVM_AudioQueueBackend table = backend(device);
    FVM_SWFSoundOutput *output = 0;
    if (!(fvm_swf_sound_output_create(mixer, &table, 16, 64, &output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.output = output;
    FVM_SWFSoundOutputStorageInfo before, after;
    before.struct_size = sizeof(before);
    after.struct_size = sizeof(after);
    if (!(fvm_swf_sound_output_storage_info(output, &before) == FVM_OK && !before.history_bytes &&
          !before.identity_bytes && before.pcm_bytes == 64))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int active = 7;
    if (!(fvm_swf_sound_output_sound_active(output, 7, &active) == FVM_UNSUPPORTED && active == 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_set_identity_tracking(output, 2) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_set_identity_tracking(output, 1) == FVM_OK &&
          fvm_swf_sound_output_set_identity_tracking(output, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_storage_info(output, &after) == FVM_OK &&
          after.identity_bytes == 65536 * sizeof(double) &&
          after.retained_bytes == before.retained_bytes + after.identity_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_sound_active(output, 7, &active) == FVM_OK && !active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_sound_active(output, 0, &active) == FVM_INVALID_ARGUMENT && !active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_sound_active(output, 65536, &active) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.failure = 2;
    active = 9;
    if (!(fvm_swf_sound_output_sound_active(output, 7, &active) == FVM_ERROR && active == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.failure = 0;
    if (!(fvm_swf_sound_output_set_identity_tracking(output, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_storage_info(output, &after) == FVM_OK && after.retained_bytes == before.retained_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_set_identity_tracking(output, 1) == FVM_OK))
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
    if (!(fvm_swf_sound_output_set_identity_tracking(output, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    after.struct_size = 0;
    if (!(fvm_swf_sound_output_storage_info(output, &after) == FVM_INVALID_ARGUMENT && !after.struct_size))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop(output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_set_identity_tracking(output, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_output_close(output);
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
