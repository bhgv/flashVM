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
static FVM_Result finalize(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{
    ++*(unsigned *)raw;fvm_value_undefined(out);return FVM_OK;
}
static void setup(FVM_Context *c,unsigned *called)
{
    FVM_Object *object=0,*meta=0;
    assert(fvm_object_create(c,&object)==FVM_OK);assert(fvm_object_create(c,&meta)==FVM_OK);
    assert(fvm_meta_register(c,meta,"__gc",finalize,called)==FVM_OK);
    assert(fvm_table_set_metatable(c,object,meta)==FVM_OK);
}
int main()
{
    long baseline=active_allocations;bool done=false;
    for(long failure=0;failure<4096 && !done;++failure)
    {
        FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;unsigned called=0;unsigned long freed=999;
        assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
        assert(fvm_context_create(rt,&c)==FVM_OK);setup(c,&called);
        allowance=failure;FVM_Result r=fvm_gc_collect(c,&freed);allowance=-1;
        assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);done=r==FVM_OK;
        assert(fvm_gc_collect(c,0)==FVM_OK);assert(called==1);
        fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(active_allocations==baseline);
    }
    assert(done);
    {
        FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;unsigned called=0;bool threw=false;
        assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
        assert(fvm_context_create(rt,&c)==FVM_OK);setup(c,&called);allowance=0;
        try{fvm_context_destroy(c);}catch(...){threw=true;}
        allowance=-1;assert(!threw && called==1);fvm_runtime_destroy(rt);
        assert(active_allocations==baseline);
    }
    {
        FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;unsigned called=0;
        FVM_Object *object=0,*meta=0;FVM_NativeEntry entry;FVM_Value value;
        assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
        assert(fvm_context_create(rt,&c)==FVM_OK);
        assert(fvm_object_create(c,&object)==FVM_OK);assert(fvm_object_create(c,&meta)==FVM_OK);
        entry.callback=finalize;entry.userdata=&called;
        meta->metamethods[fvm_detail::string("__gc\0suffix",11)]=entry;
        fvm_value_string(&value,"nonexistent");
        meta->properties[fvm_detail::string("__gc\0suffix",11)]=value;
        assert(fvm_table_set_metatable(c,object,meta)==FVM_OK);
        assert(fvm_gc_collect(c,0)==FVM_OK && !called);
        fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(active_allocations==baseline);
    }
    return 0;
}
