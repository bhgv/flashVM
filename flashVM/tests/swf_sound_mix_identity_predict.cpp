/* Predict tracked adapter storage independently and verify exact mode budget boundaries. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_identity_fixture.hpp"


int main()
{
    for (unsigned channels = 1; channels <= 2; ++channels)
    {
        for (size_t block = 1; block <= 257; block += 128)
        {
            size_t base = 0, plain = 0, tracked = 0;
            if (!(fvm_swf_sound_mix_queue_storage_required(channels, block, &base) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_sound_mix_queue_storage_required_for_tracking(channels, block, 0, &plain) == FVM_OK &&
                  plain == base))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_sound_mix_queue_storage_required_for_tracking(channels, block, 1, &tracked) == FVM_OK &&
                  tracked == base + FVM_SWF_SOUND_MIX_QUEUE_ID_SLOTS * sizeof(double)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    size_t out = 91;
    if (!(fvm_swf_sound_mix_queue_storage_required_for_tracking(1, 2, -1, &out) == FVM_INVALID_ARGUMENT && out == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_storage_required_for_tracking(1, 2, 2, &out) == FVM_INVALID_ARGUMENT && out == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_storage_required_for_tracking(0, 2, 1, &out) == FVM_INVALID_ARGUMENT && out == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_storage_required_for_tracking(2, 0, 1, &out) == FVM_INVALID_ARGUMENT && out == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_storage_required_for_tracking(2, (size_t)-1, 1, &out) == FVM_INVALID_ARGUMENT &&
          out == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_storage_required_for_tracking(2, 2, 1, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }


    IdentityFixture f(false);
    if (!(fvm_swf_sound_mix_queue_storage_required_for_tracking(1, 2, 1, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_set_identity_tracking(f.queue, 1, out - 1) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_set_identity_tracking(f.queue, 1, out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueStorageInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mix_queue_storage_info(f.queue, &info) == FVM_OK && info.retained_bytes == out))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
