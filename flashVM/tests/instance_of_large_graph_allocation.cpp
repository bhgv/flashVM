/* Inject every allocation boundary in identity-set InstanceOf traversal and verify cleanup. */
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
#define FVM_INSTANCE_GRAPH_FIXTURE_ONLY
#include "instance_of_large_graph.cpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);graph_load(c,8);FVM_Value klass,input=graph_setup(c,64,&klass,true),out;
 bool succeeded=false;unsigned failed=0;
 for(long boundary=0;boundary<4096 && !succeeded;++boundary) {
  fvm_value_number(&out,77);allowance=boundary;FVM_Result r=fvm_exec_instance_of(c,&input,&klass,&out);allowance=-1;
  assert(!c->gc_frames && !c->avm1_execution_depth && !c->gc_native_depth);
  assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);succeeded=r==FVM_OK;
  if(succeeded)assert(out.type==FVM_VALUE_BOOLEAN && out.boolean);else{++failed;assert(out.type==FVM_VALUE_NUMBER && out.number==77);}
  assert(fvm_gc_collect(c,0)==FVM_OK);
  if(!succeeded)assert(fvm_exec_instance_of(c,&input,&klass,&out)==FVM_OK && out.boolean);
 }
 assert(succeeded && failed>64);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
