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

#include "../src/core/crash_internal.hpp"
#include "flashvm/exception.h"
#include <cstring>
static unsigned callbacks;
static void paused(FVM_Context *c,FVM_Result result,void *)
{
    ++callbacks;assert(result==FVM_EXCEPTION);FVM_Value value;
    assert(fvm_crash_argument(c,0,0,&value)==FVM_OK && value.type==FVM_VALUE_STRING);
    assert(fvm_value_string_size(&value)==3 && !std::memcmp(value.string,"a\0b",3));
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    FVM_Value arg;assert(fvm_value_string_bytes(&arg,"a\0b",3)==FVM_OK);
    const unsigned char bytes[]={0};fvm_detail::string name("oom");
    assert(fvm_crash_configure(c,FVM_CRASH_DEBUGGER,0,paused,0)==FVM_OK);
    long baseline=active_allocations;
    allowance=0;
    for(unsigned i=0;i<100;++i){FVM_CrashScope scope(c,&name,bytes,sizeof(bytes),1,&arg);}
    allowance=-1;
    assert(active_allocations==baseline && !c->crash.active);
    assert(fvm_crash_set_detail(c,FVM_CRASH_DETAIL_FORCE)==FVM_OK);
    bool success=false;unsigned failures=0;
    for(long n=0;n<512 && !success;++n)
    {
        assert(fvm_crash_clear(c)==FVM_OK);callbacks=0;
        FVM_GCRootFrame *roots=c->gc_frames;allowance=n;
        {
            FVM_CrashScope scope(c,&name,bytes,sizeof(bytes),1,&arg);
            fvm_crash_failure(c,FVM_EXCEPTION);
        }
        allowance=-1;
        assert(callbacks==1 && !c->crash.active && c->gc_frames==roots && !c->crash.busy);
        size_t count;FVM_Result original=FVM_EXCEPTION,diagnostic;
        FVM_Result result=fvm_crash_info(c,&count,&original,&diagnostic);
        assert(original==FVM_EXCEPTION && (result==FVM_OK || result==FVM_NOT_FOUND));
        success=diagnostic==FVM_OK;
        if(!success){++failures;assert(diagnostic==FVM_OUT_OF_MEMORY);}
    }
    assert(success && failures && fvm_crash_clear(c)==FVM_OK);
    /* Forced object arguments retain detached graphs across an actual collection. */
    FVM_Object *object;assert(fvm_object_create(c,&object)==FVM_OK);
    fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=object;
    {
        FVM_CrashScope scope(c,&name,bytes,sizeof(bytes),1,&arg);
        fvm_value_undefined(&arg);
        assert(fvm_gc_collect(c,0)==FVM_OK);
        bool found=false;for(size_t i=0;i<c->objects.size();++i)if(c->objects[i]==object)found=true;
        assert(found);
    }
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
