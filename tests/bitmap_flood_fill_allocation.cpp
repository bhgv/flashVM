#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstdlib>
#include <new>
static long allowance = -1;
static long active_allocations = 0;
/* Inject a single allocation boundary through every VM new/new[] site. Both
 * allocation and deallocation use malloc/free so ASan can still observe them. */
void *operator new(std::size_t size) throw(std::bad_alloc)
{
    if (allowance == 0)
        throw std::bad_alloc();
    if (allowance > 0)
        --allowance;
    void *p = std::malloc(size ? size : 1);
    if (!p)
        throw std::bad_alloc();
    ++active_allocations;
    return p;
}
void operator delete(void *p) throw()
{
    if (p) --active_allocations;
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
#define FVM_FLOOD_FILL_FIXTURE_ONLY
#include "bitmap_flood_fill.cpp"
int main()
{
    long baseline=active_allocations;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);FVM_Object *image=bitmap(c,64,64,true,0xff224466UL);
    /* A connected comb grows multiple pending spans, exercising reserve failures after prior scanlines. */
    for(unsigned y=1;y<64;y+=2)for(unsigned x=1;x<64;x+=2)write(c,image,x,y,0xff000000UL);
    FVM_BitmapState *state=fvm_bitmap_pixels(image);unsigned char original[64*64*4];std::memcpy(original,state->pixels.begin(),sizeof(original));bool done=false;
    for(long failure=0;failure<4096 && !done;++failure){allowance=failure;FVM_Result r=flood(c,image,0,0,0xffabcdefUL);allowance=-1;
        assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);done=r==FVM_OK;if(!done)assert(std::memcmp(original,state->pixels.begin(),sizeof(original))==0);assert(fvm_gc_collect(c,0)==FVM_OK);}
    assert(done);assert(read(c,image,0,0)==0xffabcdefUL);assert(read(c,image,63,62)==0xffabcdefUL);assert(read(c,image,63,63)==0xff000000UL);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(active_allocations==baseline);return 0;
}
