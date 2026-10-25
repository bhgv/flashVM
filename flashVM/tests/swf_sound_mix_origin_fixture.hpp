/* Enable bounded origin accounting on an existing empty shared epoch with prepared event voices. */
#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_TEST_MIX_ORIGIN_FIXTURE_HPP
#define FVM_TEST_MIX_ORIGIN_FIXTURE_HPP
#include "swf_stream_mix_gate_fixture.hpp"


static size_t enable_origins(GateFixture &f)
{
    size_t bytes = 0;
    if (!(fvm_swf_sound_mix_queue_storage_required_for_modes(2, 2, 1, 1, &bytes) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_set_origin_tracking(f.mix, 1, bytes) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return bytes;
}


static FVM_SWFSoundMixQueueOriginInfo origins(GateFixture &f)
{
    FVM_SWFSoundMixQueueOriginInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mix_queue_origin_info(f.mix, &info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return info;
}
#endif
