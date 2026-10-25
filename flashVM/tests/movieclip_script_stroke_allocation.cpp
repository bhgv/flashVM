#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <cstdlib>
#include <new>

static long allowance = -1;


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
    return p;
}


void operator delete(void *p) throw()
{
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

#include "script_stroke_fixture.hpp"


int main(int argc, char **argv)
{
    bool faults = !(argc > 1 && std::strcmp(argv[1], "--no-faults") == 0);
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ss_context(rt, &c);
    for (unsigned cap = 0; cap < 3; ++cap)
    {
        for (unsigned join = 0; join < 2; ++join)
        {
            FVM_Object *clip = ss_clip(c, 2, 1, 1), *data = clip->resource_result.object,
                       *active = data->properties["active"].object, *gradient = data->properties["lineGradient"].object;
            FVM_Value a[8], out;
            ss_args(a, cap, join, 8);
            bool success = false;
            unsigned failures = 0;
            for (long boundary = 0; boundary < 4096 && !success; ++boundary)
            {
                FVM_GCRootFrame *roots = c->gc_frames;
                allowance = faults ? boundary : -1;
                FVM_Result result = fvm_object_call(c, clip, "lineStyle", 8, a, &out);
                allowance = -1;
                if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (result == FVM_OK)
                {
                    success = true;
                }
                else
                {
                    if (!(result == FVM_OUT_OF_MEMORY))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    ++failures;
                    if (!(data->properties["width"].number == 4 && data->properties["lineCap"].number == 2 &&
                          data->properties["lineJoin"].number == 1 && data->properties["active"].object == active &&
                          data->properties["lineGradient"].object == gradient))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(success && (!faults || failures)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(data->properties["width"].number == 8 &&
                  data->properties.find("lineGradient") == data->properties.end()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ss_metadata(clip, 2, 1);
            ss_pixels(c, clip, 2, 1, 1);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
