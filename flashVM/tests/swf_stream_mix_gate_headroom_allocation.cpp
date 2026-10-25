/* Partial FIFO filling, retained retry and cancellation allocate no native storage after preparation. */
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

#include "swf_stream_mix_gate_fixture.hpp"


int main()
{
    GateFixture f;
    size_t n = 0;
    if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_OK && n == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_OK && n == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t before = allocations;
    allowance = 0;
    f.fifo.count = 3;
    f.fifo.fail_write = 1;
    if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_ERROR && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_write = 0;
    if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 1, &n) == FVM_OK && n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.count = 3;
    if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_OK && n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_stop_all(f.mix) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_seek(&f.output, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(allocations == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    return 0;
}
