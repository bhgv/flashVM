/* Canonical-index allocation failure preserves caller vectors and restores all native storage. */
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

#include "sound_metadata_fixture.hpp"
#include "../src/swf/sound_event_index.hpp"
int main()
{
    {
        FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
        Bytes swf=metadata_movie();assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);bool done=false,failed=false;
        for(long limit=0;limit<16 && !done;++limit)
        {
            fvm_detail::vector<size_t> out;out.push_back(999);long before=active_allocations;
            allowance=limit;FVM_Result result=fvm_swf_sound_frame_ordinals(c,1,out);allowance=-1;
            assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);done=result==FVM_OK;
            if(!done){failed=true;assert(out.size()==1 && out[0]==999);}else assert(out.size()==2 && out[0]==0 && out[1]==1);
            assert(active_allocations==before && !c->avm1_execution_depth);
        }
        assert(done && failed);fvm_context_destroy(c);fvm_runtime_destroy(rt);
    }
    assert(!active_allocations);
}
