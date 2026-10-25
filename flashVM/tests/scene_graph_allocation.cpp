/* Fail reconstruction topology copies and atomic legacy migration; retain no allocations. */
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

#include "scene_controls_fixture.hpp"


int main()
{
    for (unsigned mode = 0; mode < 2; ++mode)
    {
        bool completed = false;
        unsigned failures = 0;
        for (long limit = 0; limit < 8192 && !completed; ++limit)
        {
            FVM_Config cfg;
            FVM_Runtime *rt = 0;
            if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Context *c = tick_context(rt);
            FVM_Object *root = 0, *state = 0, *children = 0;
            SceneControlProbe p;
            if (!mode)
            {
                root = scene_control_root(c, p);
                if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && p.calls == 3))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value frame, out;
                fvm_value_number(&frame, 1);
                if (!(fvm_object_call(c, root, "gotoAndStop", 1, &frame, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                root = tick_root(c);
                FVM_Object *a = tick_child(c, root, "a", 1), *b = tick_child(c, root, "b", 2),
                           *d = tick_child(c, root, "d", 3);
                state = root->resource_result.object;
                children = state->properties["children"].object;
                state->properties.erase("slot_bias");
                children->indexed.clear();
                children->indexed[1] = tick_object(a);
                children->indexed[2] = tick_object(b);
                children->indexed[3] = tick_object(d);
            }
            allowance = limit;
            FVM_Result result = fvm_movieclip_advance(c, root, 1);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            completed = result == FVM_OK;
            if (!completed)
            {
                ++failures;
            }
            tick_clean(c);
            if (!mode)
            {
                if (!(p.calls == 3))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(children->indexed.size() == 3))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                bool migrated = state->properties.find("slot_bias") != state->properties.end();
                for (unsigned long key = 1; key <= 3; ++key)
                {
                    if (!(children->indexed.find(key + (migrated ? FVM_MOVIECLIP_CHILD_SLOT_BIAS : 0)) !=
                          children->indexed.end()))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            tick_clean(c);
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
            fvm_runtime_destroy(rt);
            if (!(!active_allocations))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(completed && failures > 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}
