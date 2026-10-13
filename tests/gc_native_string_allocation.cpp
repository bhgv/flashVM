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
static FVM_Result text(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{return fvm_value_string_bytes(out,"A\0B",3);}
int main()
{
 long baseline=active_allocations;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *o=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_object_create(c,&o)==FVM_OK);assert(fvm_gc_pin(c,o)==FVM_OK);assert(fvm_object_define_accessor_rooted(c,o,"text",text,0,0,0)==FVM_OK);bool done=false;for(long failure=0;failure<4096 && !done;++failure){FVM_Value out;allowance=failure;FVM_Result r=fvm_object_get(c,o,"text",&out);allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);done=r==FVM_OK;FVM_GCStringStats stats;assert(fvm_gc_string_stats(c,&stats)==FVM_OK && stats.buffers==(done?1:0));assert(c->string_allocations.size()==c->strings.size());if(done)assert(out.string_size==3 && !std::memcmp(out.string,"A\0B",3));assert(fvm_gc_collect(c,0)==FVM_OK);}assert(done);fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(active_allocations==baseline);return 0;
}
