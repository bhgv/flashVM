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
#define FVM_COLOR_TRANSFORM_FIXTURE_ONLY
#include "standard_color_transform.cpp"
int main() {
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    double original[]={2,3,4,0.5,10,20,30,40};FVM_Object *source=color(c,8,original);FVM_Value arg=ref(source);bool success=false;
    for(long failure=0;failure<2048 && !success;++failure) {
        FVM_Object *copy=0;allowance=failure;FVM_Result r=fvm_construct(c,"flash.geom.ColorTransform",1,&arg,&copy);allowance=-1;
        assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);success=r==FVM_OK;if(success){values(c,copy,original);assert(copy!=source);rgb(c,copy,0);values(c,source,original);}assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    assert(success);unsigned char *blob=0;size_t size=0;assert(fvm_value_serialize(c,&arg,&blob,&size)==FVM_OK);success=false;
    for(long failure=0;failure<20000 && !success;++failure) {
        FVM_Value restored;allowance=failure;FVM_Result r=fvm_value_deserialize(other,blob,size,&restored);allowance=-1;
        assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);success=r==FVM_OK;if(success){assert(fvm_gc_pin(other,restored.object)==FVM_OK);received(other,restored.object);}assert(fvm_gc_collect(other,0)==FVM_OK);values(c,source,original);
    }
    assert(success);fvm_serialized_free(blob);fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
