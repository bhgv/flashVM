/* Allocation failures during pending-initializer preparation preserve at-most-once progress and guards. */
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

#include "initializer_progress_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    FVM_Context *c=tick_context(rt);FVM_Object *root=init_root(c,8,3);InitProbe p;init_bind(c,root,p);bool completed=false;unsigned failures=0;
    for(long limit=0;limit<4096 && !completed;++limit)
    {
        p.library->swf_initializer_next=1;for(unsigned i=0;i<4;++i)p.calls[i]=0;
        allowance=limit;FVM_Result status=fvm_movieclip_advance(c,root,1);allowance=-1;
        assert(status==FVM_OK || status==FVM_OUT_OF_MEMORY);completed=status==FVM_OK;if(!completed)++failures;
        assert(p.library->swf_initializer_next>=1 && p.library->swf_initializer_next<=3 && !p.calls[0]);init_clean(c,p);
        assert(fvm_movieclip_advance(c,root,1)==FVM_OK && p.library->swf_initializer_next==3);
        assert(!p.calls[0] && p.calls[1]<=1 && p.calls[2]<=1);init_clean(c,p);
        if(completed)assert(p.calls[1]==1 && p.calls[2]==1);
        assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    assert(completed && failures>2);fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(!active_allocations);
}
