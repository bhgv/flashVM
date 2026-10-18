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

#include "flashvm/function.h"
#include "flashvm/stack.h"
#include "flashvm/callable.h"
#include "flashvm/auxlib.h"
static FVM_Result read(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{return fvm_value_string_bytes(out,"A\0B",3);}
static FVM_Result stack_read(FVM_Context *,FVM_Stack *stack,void *)
{FVM_Value value;assert(fvm_stack_clear(stack)==FVM_OK);fvm_value_string_bytes(&value,"A\0B",3);return fvm_stack_push(stack,&value);}
static int lua_read(FVM_Stack *stack)
{FVM_Result r=stack_read(0,stack,0);return r==FVM_OK?1:(int)r;}
int main()
{
    long baseline=active_allocations;
    for(int kind=0;kind<5;++kind)
    {
        bool done=false;
        for(long failure=0;failure<4096 && !done;++failure)
        {
            FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *function=0;FVM_Value out;
            assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
            assert(fvm_context_create(rt,&c)==FVM_OK);
            if(kind==0)assert(fvm_function_create_native(c,read,0,0,&function)==FVM_OK);
            else if(kind==1)assert(fvm_register_function(c,"read",read,0)==FVM_OK);
            else if(kind==2)assert(fvm_stack_register(c,"read",stack_read,0)==FVM_OK);
            else if(kind==3)assert(fvm_callable_create(c,stack_read,0,&function)==FVM_OK);
            else assert(fvm_stack_register_cfunction(c,"read",lua_read,0)==FVM_OK);
            allowance=failure;
            FVM_Result r=kind==0?fvm_function_invoke(c,function,0,0,0,&out):
                         kind==3?fvm_callable_invoke(c,function,0,0,&out):fvm_call(c,"read",0,0,&out);
            allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);done=r==FVM_OK;
            assert(c->string_allocations.size()==c->strings.size());
            FVM_GCStringStats stats;assert(fvm_gc_string_stats(c,&stats)==FVM_OK && stats.buffers==(done?1:0));
            if(done)assert(out.string_size==3 && !std::memcmp(out.string,"A\0B",3));
            assert(fvm_gc_collect(c,0)==FVM_OK);
            fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(active_allocations==baseline);
        }
        assert(done);
    }
    return 0;
}
