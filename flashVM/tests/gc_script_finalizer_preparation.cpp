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
#include "flashvm/gc_roots.h"
#include "../src/core/runtime.hpp"
#include <cstring>

#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include "flashvm/avm1.h"
static FVM_Result tick(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{
    ++*(unsigned *)raw;fvm_value_number(out,1);return FVM_OK;
}
static const unsigned char code[]={
  0x9b,29,0,'c','l','e','a','n','u','p',0,0,0,17,0,
  0x96,11,0,7,0,0,0,0,0,'t','i','c','k',0,0x3d,0x17,0,0
};
static void setup(FVM_Context *c,unsigned *called,int named,FVM_Object **object)
{
    FVM_Object *meta=0;FVM_Value finalizer;
    assert(fvm_register_function(c,"tick",tick,called)==FVM_OK);
    assert(fvm_avm1_execute(c,code,sizeof(code))==FVM_OK);
    assert(fvm_object_create(c,object)==FVM_OK);assert(fvm_object_create(c,&meta)==FVM_OK);
    if(named)assert(fvm_value_string(&finalizer,"cleanup")==FVM_OK);
    else assert(fvm_get_global(c,"cleanup",&finalizer)==FVM_OK && finalizer.type==FVM_VALUE_OBJECT);
    assert(fvm_object_set(c,meta,"__gc",&finalizer)==FVM_OK);
    assert(fvm_table_set_metatable(c,*object,meta)==FVM_OK);
}
int main()
{
    long baseline=active_allocations;
    for(int collecting=0;collecting<2;++collecting)
    for(int named=0;named<2;++named)
    {
        bool done=false,saw_preparation_failure=false,saw_body_failure=false;
        for(long failure=0;failure<4096 && !done;++failure)
        {
            FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *object=0;unsigned called=0;
            assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
            assert(fvm_context_create(rt,&c)==FVM_OK);setup(c,&called,named,&object);
            allowance=failure;FVM_Result r=collecting?fvm_gc_collect(c,0):fvm_meta_finalize(c,object);allowance=-1;
            assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);done=r==FVM_OK;
            bool present=false;
            for(size_t i=0;i<c->objects.size();++i)if(c->objects[i]==object)present=true;
            assert(present && !c->gc_collecting && !c->pending_finalizer && !c->gc_native_depth);
            if(!failure)assert(r==FVM_OUT_OF_MEMORY && !object->finalized && !called);
            assert(c->script_depth==0 && c->gc_frames==0 && c->lexical_scopes.empty());
            bool started=object->finalized;unsigned previous=called;
            if(r==FVM_OUT_OF_MEMORY && !started)saw_preparation_failure=true;
            if(r==FVM_OUT_OF_MEMORY && started)saw_body_failure=true;
            assert(fvm_meta_finalize(c,object)==FVM_OK);
            assert(called==(started?previous:1) && object->finalized);
            assert(fvm_gc_collect(c,0)==FVM_OK);
            fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(active_allocations==baseline);
        }
        assert(done && saw_preparation_failure && saw_body_failure);
    }
    return 0;
}
