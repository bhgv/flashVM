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
#define FVM_BITMAP_DRAW_FIXTURE_ONLY
#include "bitmap_draw.cpp"
int main()
{
    long baseline=active_allocations;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);FVM_Object *source=bitmap(c,2,1,true,0),*dest=0;write(c,source,1,0,0x80ff0000UL);
    FVM_Object *clip=0;assert(fvm_movieclip_create(c,"source",1,&clip)==FVM_OK);assert(fvm_gc_pin(c,clip)==FVM_OK);FVM_Value fill[2];fvm_value_number(fill,0xff0000);fvm_value_number(fill+1,0);call(c,clip,"beginFill",2,fill);geometry(c,clip,true);
    FVM_Object *nested=child(c,clip,"child",1);fill_color(c,nested,0x00ff00UL);set_number(c,nested,"_x",2);set_number(c,nested,"_alpha",50);
    for(unsigned kind=0;kind<6;++kind){dest=bitmap(c,8,8,true,0xc8010203UL);bool done=false;unsigned char original[256];FVM_BitmapState *target=fvm_bitmap_pixels(dest);std::memcpy(original,target->pixels.begin(),256);
        for(long failure=0;failure<4096 && !done;++failure){allowance=failure;FVM_Result r=draw_result(c,dest,(kind%2)?clip:source,0,0,kind/2==0?1:kind/2==1?11:12);allowance=-1;
            assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY || r==FVM_UNSUPPORTED);done=r==FVM_OK || r==FVM_UNSUPPORTED;
            if(!done)assert(std::memcmp(original,target->pixels.begin(),256)==0);assert(fvm_gc_collect(c,0)==FVM_OK);}assert(done);}
    fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(active_allocations==baseline);return 0;
}
