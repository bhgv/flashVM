#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstdlib>
#include <new>
static long allowance = -1;
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
    return p;
}
void operator delete(void *p) throw()
{
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
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/avm1.h"
#include "../src/core/runtime.hpp"
static unsigned entered;
static FVM_Result generated(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++entered;
    return fvm_exec_numeric(c, FVM_EXEC_ADD, &frame->argv[0], &frame->argv[0], out);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; FVM_Object *fn=0;
    const unsigned char code[]={0x96,5,0,7,7,0,0,0,0x3e,0};
    const char *parameters[]={"x"}; FVM_Value arg,out,left,right;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    assert(fvm_function_create_avm1(c,code,sizeof(code),1,parameters,0,&fn)==FVM_OK);
    assert(fvm_gc_pin(c,fn)==FVM_OK && fvm_function_set_body(c,fn,generated,0,0)==FVM_OK);
    fvm_value_number(&arg,21); fvm_value_string(&left,"hello"); fvm_value_string(&right," world");
    bool success=false; unsigned failures=0;
    for(long boundary=0; boundary<2000 && !success; ++boundary)
    {
        FVM_GCRootFrame *roots=c->gc_frames; unsigned depth=c->script_depth, native=c->gc_native_depth;
        size_t scopes=c->lexical_scopes.size(); FVM_Object *self=c->script_this;
        bool target=c->avm1_target_active; FVM_Object *base=c->avm1_base_clip,*selected=c->avm1_target_clip;
        allowance=boundary;
        FVM_Result r=fvm_function_invoke(c,fn,0,1,&arg,&out);
        allowance=-1;
        assert(c->gc_frames==roots && c->script_depth==depth && c->gc_native_depth==native);
        assert(c->lexical_scopes.size()==scopes && c->script_this==self);
        assert(c->avm1_target_active==target && c->avm1_base_clip==base && c->avm1_target_clip==selected);
        if(r==FVM_OK) { assert(out.type==FVM_VALUE_NUMBER && out.number==42); success=true; }
        else { assert(r==FVM_OUT_OF_MEMORY); ++failures; }
        assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    assert(success && failures>10 && entered);
    success=false; failures=0;
    for(long boundary=0; boundary<200 && !success; ++boundary)
    {
        FVM_GCRootFrame *roots=c->gc_frames; fvm_value_number(&out,99);
        allowance=boundary;
        FVM_Result r=fvm_exec_compare(c,FVM_EXEC_ADD_DYNAMIC,&left,&right,&out);
        allowance=-1;
        assert(c->gc_frames==roots);
        if(r==FVM_OK) { assert(out.type==FVM_VALUE_STRING); success=true; }
        else { assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==99); ++failures; }
    }
    assert(success && failures);
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
