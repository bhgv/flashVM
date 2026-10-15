/* Inject every allocation boundary in indexed display bounds and verify cleanup. */
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
#define FVM_INDEXED_SIZE_FIXTURE_ONLY
#include "movieclip_indexed_size.cpp"
int main()
{
 FVM_Config cfg; FVM_Runtime *rt=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned index=8;index<=9;++index) {
  bool succeeded=false;
  for(long failure=0;failure<4096 && !succeeded;++failure) {
   FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
   FVM_Object *root=0;assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
   size_rect(c,root,0,0,10,20);FVM_Object *nested=child(c,root,"nested",2);size_rect(c,nested,30,-10,5,5);
   FVM_Value target=size_object(root),at,out;fvm_value_number(&at,index);fvm_value_number(&out,77);
   allowance=failure;FVM_Result r=fvm_exec_display_property(c,0,&target,&at,0,&out);allowance=-1;
   assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);succeeded=r==FVM_OK;
   assert(c->gc_frames==0 && c->gc_native_depth==0 && c->avm1_execution_depth==0);
   if(!succeeded)assert(out.type==FVM_VALUE_NUMBER && out.number==77);
   assert(fvm_gc_collect(c,0)==FVM_OK);size_expect(c,root,index,index==8?35:30);
   fvm_context_destroy(c);
  }
  assert(succeeded);
 }
 fvm_runtime_destroy(rt);
}
