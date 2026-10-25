/* Actual FIFO dequeue splits silent-valued BASE and TAIL frames by admission origin, not PCM. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_origin_fixture.hpp"


int main()
{
    GateFixture f;
    FVM_SWFSoundMixQueueOriginInfo info;
    info.struct_size = sizeof(info);
    info.base_submitted_frames = 91;
    if (!(fvm_swf_sound_mix_queue_origin_info(f.mix, &info) == FVM_UNSUPPORTED && info.base_submitted_frames == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t bytes = enable_origins(f);
    if (!(fvm_swf_sound_mix_queue_set_origin_tracking(f.mix, 1, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short base[4] = {0, 0, 0, 0};
    size_t n = 0;
    if (!(f.adapted.write(f.adapted.userdata, base, 2) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_pump_tail_finite(f.mix, 2, 4, &n) == FVM_OK && n == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info = origins(f);
    if (!(info.base_submitted_frames == 2 && info.tail_submitted_frames == 2 && info.base_queued_frames == 2 &&
          info.tail_queued_frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.count = 3;
    info = origins(f);
    if (!(info.base_played_frames == 1 && !info.tail_played_frames && info.base_queued_frames == 1 &&
          info.tail_queued_frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.count = 1;
    info = origins(f);
    if (!(info.base_played_frames == 2 && info.tail_played_frames == 1 && !info.base_queued_frames &&
          info.tail_queued_frames == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_set_origin_tracking(f.mix, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.count = 0;
    info = origins(f);
    if (!(info.base_played_frames == 2 && info.tail_played_frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_clock_begin(f.mix) == FVM_INVALID_ARGUMENT &&
          fvm_swf_sound_mix_queue_clock_end(f.mix) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_set_origin_tracking(f.mix, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueStorageInfo storage;
    storage.struct_size = sizeof(storage);
    if (!(fvm_swf_sound_mix_queue_storage_info(f.mix, &storage) == FVM_OK && storage.retained_bytes == bytes - 32768))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
