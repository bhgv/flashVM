#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstdlib>
#include <new>
static long allowance = -1;
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
#define FVM_TRANSFORM_FIXTURE_ONLY
#include "standard_transform.cpp"
int main() {
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    FVM_Object *root=0;assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK);assert(fvm_gc_pin(c,root)==FVM_OK);FVM_Object *t=property(c,root,"transform");
    double before[]={1,1,1,0.5,1,2,3,4},after[]={0.25,0.5,0.75,1.5,10,20,30,-100};
    FVM_Object *first=construct(c,"flash.geom.ColorTransform",8,before),*next=construct(c,"flash.geom.ColorTransform",8,after);assign(c,t,"colorTransform",first);FVM_Value arg;ref(&arg,next);bool success=false;
    for(long failure=0;failure<2048 && !success;++failure) {
        allowance=failure;FVM_Result r=fvm_object_set(c,t,"colorTransform",&arg);allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);success=r==FVM_OK;
        color(c,property(c,t,"colorTransform"),success?after:before);near(num(c,root,"_alpha"),success?150:50);assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    assert(success);
    const char *keys[]={"matrix","colorTransform","concatenatedMatrix","concatenatedColorTransform","pixelBounds","transform"};
    for(unsigned i=0;i<6;++i) {
        success=false;
        for(long failure=0;failure<2048 && !success;++failure) {
            FVM_Value value;allowance=failure;FVM_Result r=fvm_object_get(c,i==5?root:t,keys[i],&value);allowance=-1;
            assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);success=r==FVM_OK;if(success)assert(value.type==FVM_VALUE_OBJECT);assert(fvm_gc_collect(c,0)==FVM_OK);near(num(c,root,"_alpha"),150);
        }
        assert(success);
    }
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
