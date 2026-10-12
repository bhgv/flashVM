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
#define FVM_GEOMETRY_FIXTURE_ONLY
#include "standard_geometry.cpp"
int main() {
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    double xy[]={3,4}; FVM_Object *p=make(c,"flash.geom.Point",2,xy); bool success=false;
    for(long failure=0;failure<2048 && !success;++failure) {
        FVM_Value out; allowance=failure;
        FVM_Result r=fvm_object_call(c,p,"clone",0,0,&out); allowance=-1;
        assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY); success=r==FVM_OK;
        if(success) { near(get(c,out.object,"x"),3); assert(out.object!=p); }
        near(get(c,p,"x"),3); near(get(c,p,"y"),4); assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    assert(success);
    FVM_Value text; fvm_value_string(&text,"prefix"); assert(fvm_object_set(c,p,"x",&text)==FVM_OK);
    FVM_Value operand=ref(p); success=false;
    for(long failure=0;failure<2048 && !success;++failure) {
        FVM_Value out; allowance=failure;
        FVM_Result r=fvm_object_call(c,p,"add",1,&operand,&out); allowance=-1;
        assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY); success=r==FVM_OK;
        if(success) { FVM_Value joined; assert(fvm_object_get(c,out.object,"x",&joined)==FVM_OK); assert(std::strcmp(joined.string,"prefixprefix")==0); }
        FVM_Value original; assert(fvm_object_get(c,p,"x",&original)==FVM_OK); assert(std::strcmp(original.string,"prefix")==0);
        assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    assert(success); fvm_context_destroy(c);
    success=false;
    for(long failure=0;failure<20000 && !success;++failure) {
        assert(fvm_context_create(rt,&c)==FVM_OK); allowance=failure;
        FVM_Result r=fvm_standard_install(c); allowance=-1;
        assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY); success=r==FVM_OK;
        FVM_Value global; FVM_Result found=fvm_get_global(c,"flash",&global);
        if(!success) assert(found==FVM_NOT_FOUND || global.type==FVM_VALUE_UNDEFINED);
        else { assert(found==FVM_OK); assert(fvm_standard_install(c)==FVM_OK); }
        assert(fvm_gc_collect(c,0)==FVM_OK); fvm_context_destroy(c);
    }
    assert(success); fvm_runtime_destroy(rt); return 0;
}
