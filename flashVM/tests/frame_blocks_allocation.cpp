/* Allocation failures during frame preparation restore roots/work/debug state and leave reusable code. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <cstdlib>
#include <new>

static long allowance = -1;
static long active_allocations = 0;


/* Inject a single allocation boundary through every VM new/new[] site. Both
 * allocation and deallocation use malloc/free so ASan can still observe them. */
void *operator new(std::size_t size) throw(std::bad_alloc)
{
    if (allowance == 0)
    {
        throw std::bad_alloc();
    }
    if (allowance > 0)
    {
        --allowance;
    }
    void *p = std::malloc(size ? size : 1);
    if (!p)
    {
        throw std::bad_alloc();
    }
    ++active_allocations;
    return p;
}


void operator delete(void *p) throw()
{
    if (p)
    {
        --active_allocations;
    }
    std::free(p);
}


void *operator new[](std::size_t size) throw(std::bad_alloc)
{
    return ::operator new(size);
}


void operator delete[](void *p) throw()
{
    ::operator delete(p);
}


void *operator new(std::size_t size, const std::nothrow_t &) throw()
{
    try
    {
        return ::operator new(size);
    }
    catch (...)
    {
        return 0;
    }
}


void operator delete(void *p, const std::nothrow_t &) throw()
{
    ::operator delete(p);
}


void *operator new[](std::size_t size, const std::nothrow_t &) throw()
{
    try
    {
        return ::operator new[](size);
    }
    catch (...)
    {
        return 0;
    }
}


void operator delete[](void *p, const std::nothrow_t &) throw()
{
    ::operator delete[](p);
}

#define FVM_BITMAP_FILL_FIXTURE_ONLY
#include "movieclip_bitmap_fill.cpp"

#include "frame_blocks_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned route = 0; route < 2; ++route)
    {
        FVM_Context *c = tick_context(rt);
        FVM_Object *root = tick_root(c), *segments = frame_segments(c, root);
        for (unsigned i = 0; i < 3; ++i)
        {
            segments->indexed[i] = frame_hex(c, action(i + 1));
        }
        c->debug_function = "caller";
        c->debug_offset = 47;
        bool success = false;
        unsigned failures = 0;
        for (long limit = 0; limit < 10000 && !success; ++limit)
        {
            allowance = limit;
            FVM_Result result = run_frame(c, root, route);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            success = result == FVM_OK;
            if (!success)
            {
                ++failures;
            }
            if (!(c->debug_function == "caller" && c->debug_offset == 47))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            tick_clean(c);
        }
        if (!(success && failures > 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, root, "seen", 3);
        if (!(fvm_gc_collect(c, 0) == FVM_OK && run_frame(c, root, route) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    if (!(!active_allocations))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
