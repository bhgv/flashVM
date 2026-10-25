/* Origin predictor accounts exact optional bitmap bytes and preserves output on invalid modes. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_mix_gate_fixture.hpp"


int main()
{
    size_t base = 0, ids = 0, origins = 0, both = 0;
    if (!(fvm_swf_sound_mix_queue_storage_required(2, 2, &base) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_storage_required_for_tracking(2, 2, 1, &ids) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_storage_required_for_modes(2, 2, 0, 1, &origins) == FVM_OK &&
          origins - base == 32768))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_storage_required_for_modes(2, 2, 1, 1, &both) == FVM_OK && both - ids == 32768))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t old = both;
    if (!(fvm_swf_sound_mix_queue_storage_required_for_modes(2, 2, 1, 2, &both) == FVM_INVALID_ARGUMENT && both == old))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_storage_required_for_modes(2, 2, 2, 1, &both) == FVM_INVALID_ARGUMENT && both == old))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    GateFixture f;
    FVM_SWFSoundMixQueueStorageInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mix_queue_storage_info(f.mix, &info) == FVM_OK && info.retained_bytes == ids))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
