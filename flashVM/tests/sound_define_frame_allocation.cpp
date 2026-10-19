/* Action replacement preserves imported sound metadata atomically across allocation failures. */
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

#include "sound_snapshot_fixture.hpp"
int main()
{
    bool completed=false,failed=false;
    for(long limit=0;limit<256 && !completed;++limit)
    {
        FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
        FVM_Context *c=tick_context(rt);SoundSnapshot p;FVM_Object *root=sound_snapshot_root(c,p);
        FVM_Object *frames=root->resource_result.object->properties["frames"].object,*before=frames->indexed[1].object;
        const unsigned char end[]={0};allowance=limit;FVM_Result result=fvm_movieclip_define_frame(c,root,1,"replacement",end,sizeof(end));allowance=-1;
        assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);completed=result==FVM_OK;
        if(!completed){failed=true;assert(frames->indexed[1].object==before);}
        assert(frames->indexed[1].object->properties["sounds"].object==p.list);
        assert(fvm_movieclip_advance(c,root,1)==FVM_OK && p.count==2);tick_clean(c);assert(!c->movieclip_sound_busy);
        assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(!active_allocations);
    }
    assert(completed && failed);
}
