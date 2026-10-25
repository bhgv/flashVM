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

#define FVM_PNG_GIF_FIXTURE_ONLY
#include "image_codecs_swf_png_gif.cpp"


int main()
{
    Bytes sources[2] = {png_fixture(), gif_fixture(true)};
    for (unsigned variant = 0; variant < 2; ++variant)
    {
        Bytes payload = jpeg_resource(sources[variant]);
        bool done = false;
        for (long failure = 0; failure < 2048 && !done; ++failure)
        {
            FVM_SWFBitmap decoded;
            decoded.id = 777;
            allowance = failure;
            FVM_Result r = fvm_swf_read_jpeg(payload.begin(), payload.size(), 21, 8, 1024, decoded);
            allowance = -1;
            if (!(r == FVM_OK || r == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            done = r == FVM_OK;
            if (!done)
            {
                if (!(decoded.id == 777 && decoded.pixels.empty()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (!(done))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
