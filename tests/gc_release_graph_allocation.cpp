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

#include "flashvm/stack.h"
#include "flashvm/userdata.h"
#include "flashvm/table.h"
struct Payload {FVM_Stack *stack;FVM_Object *object;unsigned released;};
static void release(void *raw)
{
    Payload *p=(Payload *)raw;FVM_Value v;++p->released;
    fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=p->object;
    assert(fvm_stack_push(p->stack,&v)==FVM_OK);
}
int main()
{
    long baseline=active_allocations;bool done=false,saw_late_failure=false;
    for(long failure=0;failure<4096 && !done;++failure)
    {
        FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;Payload p;FVM_Value v,child,value;
        FVM_GCStringStats stats;unsigned long freed=999;p.released=0;
        assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
        assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
        assert(fvm_stack_create(c,&p.stack)==FVM_OK);fvm_value_undefined(&v);
        for(unsigned i=0;i<8;++i)assert(fvm_stack_push(p.stack,&v)==FVM_OK);
        assert(fvm_stack_clear(p.stack)==FVM_OK);
        assert(fvm_object_create(c,&p.object)==FVM_OK);assert(fvm_userdata_attach(c,p.object,&p,release)==FVM_OK);
        for(unsigned i=0;i<50;++i)
        {
            FVM_Object *object=0;assert(fvm_object_create(c,&object)==FVM_OK);
            fvm_value_undefined(&child);child.type=FVM_VALUE_OBJECT;child.object=object;
            assert(fvm_table_set_index(c,p.object,i,&child)==FVM_OK);
            assert(fvm_value_string_bytes(&value,"A\0B",3)==FVM_OK);
            assert(fvm_object_set(c,object,"text",&value)==FVM_OK);
        }
        allowance=failure;FVM_Result r=fvm_gc_collect(c,&freed);allowance=-1;
        assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);done=r==FVM_OK;
        assert(!c->gc_collecting && !freed);
        assert(fvm_gc_string_stats(c,&stats)==FVM_OK && stats.buffers==50);
        assert(c->string_allocations.size()==c->strings.size());
        if(r==FVM_OUT_OF_MEMORY && p.released)saw_late_failure=true;
        assert(fvm_gc_collect(c,0)==FVM_OK && p.released==1);
        assert(fvm_stack_get(p.stack,1,&v)==FVM_OK && v.object==p.object);
        assert(fvm_table_get_index(c,v.object,49,&child)==FVM_OK);
        assert(fvm_object_get(c,child.object,"text",&value)==FVM_OK && !std::memcmp(value.string,"A\0B",3));
        assert(fvm_stack_clear(p.stack)==FVM_OK);assert(fvm_gc_collect(c,&freed)==FVM_OK && freed==51);
        assert(fvm_gc_string_stats(c,&stats)==FVM_OK && !stats.buffers);
        fvm_stack_destroy(p.stack);fvm_context_destroy(c);fvm_runtime_destroy(rt);
        assert(active_allocations==baseline);
    }
    assert(done && saw_late_failure);return 0;
}
