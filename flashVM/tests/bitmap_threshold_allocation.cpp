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
#define FVM_THRESHOLD_FIXTURE_ONLY
#include "bitmap_threshold.cpp"
int main()
{
    long baseline=active_allocations;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);FVM_Object *image=bitmap(c,4,1,true,1);for(unsigned x=0;x<4;++x)write(c,image,x,0,x+1);
    double box[]={0,0,3,1},xy[]={1,0};FVM_Object *rect=region_object(c,"flash.geom.Rectangle",4,box),*point=region_object(c,"flash.geom.Point",2,xy);FVM_Value a[8];threshold_args(a,image,rect,point,"==",0,0,0xffffffffUL,true);
    unsigned char original[16];FVM_BitmapState *pixels=fvm_bitmap_pixels(image);std::memcpy(original,pixels->pixels.begin(),16);bool done=false;
    for(long failure=0;failure<2048 && !done;++failure){FVM_Value out;allowance=failure;FVM_Result r=fvm_object_call(c,image,"threshold",8,a,&out);allowance=-1;
        assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);done=r==FVM_OK;if(!done)assert(std::memcmp(original,pixels->pixels.begin(),16)==0);assert(fvm_gc_collect(c,0)==FVM_OK);}assert(done);
    assert(read(c,image,1,0)==1 && read(c,image,2,0)==2 && read(c,image,3,0)==3);fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(active_allocations==baseline);return 0;
}
