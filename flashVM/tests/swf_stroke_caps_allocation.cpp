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

#include "swf_stroke_caps_fixture.hpp"


int main(int argc, char **argv)
{
    bool faults = !(argc > 1 && std::strcmp(argv[1], "--no-faults") == 0);
    for (unsigned mode = 0; mode < 24; ++mode)
    {
        unsigned type = mode / 6, cap = (mode / 2) % 3, join = mode % 2;
        Bytes source = caps_shape(type, cap, join, 80, true, true);
        bool complete = false;
        for (long failure = 0; failure < 4096 && !complete; ++failure)
        {
            FVM_SWFShape out;
            out.id = 777;
            allowance = faults ? failure : -1;
            FVM_Result result = fvm_swf_read_shape(source.begin(), source.size(), 83, out);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            complete = result == FVM_OK;
            if (!complete)
            {
                if (!(out.id == 777 && out.paints.empty()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (!(complete))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Config cfg;
        FVM_Runtime *rt = 0;
        FVM_Context *c = 0;
        if (!(fvm_config_init(&cfg) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes bytes = movie(bs_tags(source), 1);
        complete = false;
        for (long failure = 0; failure < 8192 && !complete; ++failure)
        {
            FVM_Object *root = 0;
            allowance = faults ? failure : -1;
            FVM_Result result = fvm_movieclip_load_swf(c, bytes.begin(), bytes.size(), "root", &root);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            complete = result == FVM_OK;
            if (!complete)
            {
                if (!(!root))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(root))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_pin(c, root) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                tick(c, root);
                caps_pixels(c, root, type, cap, join, 80, true, true);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(complete))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *root = load(c, bs_tags(source), 1);
        tick(c, root);
        complete = false;
        unsigned char pixels[1024], original[1024];
        FVM_RasterSurface surface = {pixels, 16, 16, 64};
        for (long failure = 0; failure < 4096 && !complete; ++failure)
        {
            std::memset(pixels, 37, sizeof(pixels));
            std::memcpy(original, pixels, sizeof(pixels));
            allowance = faults ? failure : -1;
            FVM_Result result = fvm_movieclip_render(c, root, &surface);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY || result == FVM_UNSUPPORTED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            complete = result != FVM_OUT_OF_MEMORY;
            if (result != FVM_OK)
            {
                if (!(std::memcmp(pixels, original, sizeof(pixels)) == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (!(complete))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
    }
    return 0;
}
