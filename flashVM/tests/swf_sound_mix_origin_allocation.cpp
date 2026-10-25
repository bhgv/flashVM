/* Gate scheduling, retained retry and cancellation allocate no native storage after preparation. */
#include <stdio.h>
#include <stdlib.h>
#include <new>
#include <cstdlib>

static long allowance = -1;
static size_t allocations = 0;


void *operator new(std::size_t n) throw(std::bad_alloc)
{
    ++allocations;
    if (!allowance)
    {
        throw std::bad_alloc();
    }
    if (allowance > 0)
    {
        --allowance;
    }
    void *p = std::malloc(n ? n : 1);
    if (!p)
    {
        throw std::bad_alloc();
    }
    return p;
}


void operator delete(void *p) throw()
{
    std::free(p);
}


void *operator new[](std::size_t n) throw(std::bad_alloc)
{
    return ::operator new(n);
}


void operator delete[](void *p) throw()
{
    ::operator delete(p);
}

#include "swf_sound_mix_origin_fixture.hpp"


int main()
{
    GateFixture f;
    size_t budget = 0;
    if (!(fvm_swf_sound_mix_queue_storage_required_for_modes(2, 2, 1, 1, &budget) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = 0;
    if (!(fvm_swf_sound_mix_queue_set_origin_tracking(f.mix, 1, budget) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    FVM_SWFSoundMixQueueOriginInfo info;
    info.struct_size = sizeof(info);
    info.base_submitted_frames = 91;
    if (!(fvm_swf_sound_mix_queue_origin_info(f.mix, &info) == FVM_UNSUPPORTED && info.base_submitted_frames == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    enable_origins(f);
    size_t before = allocations;
    allowance = 0;
    size_t n = 0;
    for (unsigned i = 0; i < 20; ++i)
    {
        f.fifo.count = 0;
        f.fifo.fail_write = 1;
        FVM_Result r = fvm_swf_stream_output_pump_mix_finite_until(&f.output, f.mix, 1, &n);
        if (r == FVM_NOT_FOUND)
        {
            break;
        }
        if (!(r == FVM_ERROR && !n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.fifo.fail_write = 0;
        if (!(fvm_swf_stream_output_pump_mix_finite_until(&f.output, f.mix, 1, &n) == FVM_OK && n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        info = origins(f);
        FVM_SWFStreamMixInfo paired;
        paired.struct_size = sizeof(paired);
        if (!(fvm_swf_stream_output_mix_info(&f.output, f.mix, &paired) == FVM_OK &&
              paired.stream.submitted_frames ==
                  paired.origins.base_submitted_frames + paired.origins.tail_submitted_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    f.fifo.fail_write = 0;
    f.fifo.count = 0;
    info = origins(f);
    if (!(info.base_played_frames == 9 && info.tail_played_frames == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_seek(&f.output, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info = origins(f);
    if (!(!info.base_submitted_frames && !info.tail_submitted_frames && allocations == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    return 0;
}
