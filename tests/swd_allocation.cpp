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

#define FVM_SWD_FIXTURE_ONLY
#include "swd_sources.cpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    Bytes swf,swd;size_t start=movie(swf);symbols(swd,start);
    assert(fvm_debug_load_swd(c,swf.data.begin(),swf.data.size(),swd.data.begin(),swd.data.size())==FVM_OK);
    FVM_Value original;fvm_value_number(&original,77);assert(fvm_exception_raise(c,&original)==FVM_EXCEPTION);
    bool success=false;unsigned failures=0;
    for(long n=0;n<4096 && !success;++n)
    {
        allowance=n;
        FVM_Result result=fvm_debug_load_swd(c,swf.data.begin(),swf.data.size(),swd.data.begin(),swd.data.size());
        allowance=-1;
        assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);success=result==FVM_OK;
        if(!success)++failures;
        size_t count;assert(fvm_debug_source_count(c,&count)==FVM_OK && count==1);
        FVM_DebugSourcePosition p;assert(fvm_debug_source_position(c,code,sizeof(code),8,&p)==FVM_OK && p.line==2);
        assert(fvm_exception_get(c,&original)==FVM_OK && original.number==77);
    }
    assert(success && failures && fvm_exception_clear(c)==FVM_OK);
    assert(fvm_debug_clear_symbols(c)==FVM_OK);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
