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

#include "swf_sound_output_finite_fixture.hpp"


int main()
{


    FiniteOutput f(8, true);
    f.start(0, true);
    size_t n = 0;
    size_t before = allocations;
    allowance = 0;
    for (unsigned i = 0; i < 3; ++i)
    {
        f.device.queued = 0;
        f.device.failure = 1;
        if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_ERROR && !n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        f.device.failure = 0;
        if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int pending = 0;
        if (!(fvm_swf_sound_output_pending(f.output, &pending) == FVM_OK && pending))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(f.device.values == 18))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.device.queued = 0;
    int pending = 91;
    if (!(fvm_swf_sound_output_pending(f.output, &pending) == FVM_OK && !pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop(f.output) == FVM_OK && allocations == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    return 0;
}
