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
int main()
{
    long baseline=active_allocations;bool done=false;
    for(long failure=0;failure<4096 && !done;++failure)
    {
        FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Value value;
        unsigned short bytes[3]={'A',0,'B'};FVM_GCStringStats before,after;
        assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
        assert(fvm_context_create(rt,&c)==FVM_OK);
        assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
        assert(fvm_value_string_utf16(c,bytes,3,&value)==FVM_OK);
        assert(fvm_set_global(c,"live",&value)==FVM_OK);
        assert(fvm_value_string_utf16(c,bytes,3,&value)==FVM_OK);
        assert(fvm_gc_string_stats(c,&before)==FVM_OK && before.buffers==3);
        allowance=failure;FVM_Result r=fvm_gc_collect(c,0);allowance=-1;
        assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);done=r==FVM_OK;
        assert(!c->gc_collecting && c->string_allocations.size()==c->strings.size());
        assert(fvm_gc_string_stats(c,&after)==FVM_OK && after.buffers==(done?1:3));
        assert(fvm_get_global(c,"live",&value)==FVM_OK && !std::memcmp(value.string,"A\0B",3));
        fvm_value_undefined(&value);assert(fvm_set_global(c,"live",&value)==FVM_OK);
        assert(fvm_gc_collect(c,0)==FVM_OK);
        assert(fvm_gc_string_stats(c,&after)==FVM_OK && !after.buffers);
        assert(c->string_allocations.empty());
        fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(active_allocations==baseline);
    }
    assert(done);return 0;
}
