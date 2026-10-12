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
#define FVM_HIT_FIXTURE_ONLY
#include "bitmap_hit_test.cpp"
int main()
{
    long baseline=active_allocations;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    FVM_Object *image=bitmap(c,3,2,true,0x80000000UL),*first=hit_point(c,0,0),*comparison=bitmap(c,3,2,true,0x80000000UL);double xywh[]={0,0,3,2};FVM_Object *rect=region_object(c,"flash.geom.Rectangle",4,xywh);FVM_Object *cases[]={first,rect,comparison};
    for(unsigned i=0;i<3;++i){FVM_Value a[5];hit_args(a,first,128,cases[i],first,128);bool done=false;for(long failure=0;failure<4096 && !done;++failure){FVM_Value out;allowance=failure;FVM_Result r=fvm_object_call(c,image,"hitTest",5,a,&out);allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);done=r==FVM_OK;if(done)assert(out.type==FVM_VALUE_BOOLEAN && out.boolean);assert(read(c,image,0,0)==0x80000000UL);assert(fvm_gc_collect(c,0)==FVM_OK);}assert(done);}
    fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(active_allocations==baseline);return 0;
}
