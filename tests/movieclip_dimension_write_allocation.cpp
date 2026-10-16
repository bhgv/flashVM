/* Inject every allocation boundary in atomic indexed dimension writes and verify cleanup. */
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
#define FVM_DIMENSION_WRITE_FIXTURE_ONLY
#include "movieclip_dimension_write.cpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned index=8;index<=9;++index){bool succeeded=false;unsigned failures=0;
  for(long boundary=0;boundary<4096 && !succeeded;++boundary){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);dimension_load(c,8);FVM_Object *root=0;assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
   size_rect(c,root,0,0,10,20);size_scalar(c,root,"_xscale",130);size_scalar(c,root,"_yscale",170);FVM_Value target=size_object(root),at,value,out;fvm_value_number(&at,index);fvm_value_number(&value,40);fvm_value_number(&out,77);
   allowance=boundary;FVM_Result r=fvm_exec_display_property(c,1,&target,&at,&value,&out);allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);succeeded=r==FVM_OK;
   if(!succeeded){++failures;assert(out.type==FVM_VALUE_NUMBER && out.number==77);dimension_scale(c,root,130,170);}else dimension_scale(c,root,index==8?400:130,index==8?170:200);
   assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth && fvm_gc_collect(c,0)==FVM_OK);dimension_write(c,root,index,40);size_expect(c,root,index,40);fvm_context_destroy(c);
  }
  assert(succeeded && failures);
 }
 fvm_runtime_destroy(rt);
}
