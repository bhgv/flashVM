/* Inject every allocation boundary in charged split and search and verify cleanup. */
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
#define FVM_STRING_WORK_FIXTURE_ONLY
#include "string_loaded_work.cpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);string_work_load(c,8);FVM_Object *str=string_work_make(c,"a,b,c",5);
 const char *methods[]={"split","indexOf","lastIndexOf"};
 for(unsigned m=0;m<3;++m){bool succeeded=false;unsigned failures=0;FVM_Value arg,out;fvm_value_string(&arg,",");
  for(long boundary=0;boundary<4096 && !succeeded;++boundary){allowance=boundary;FVM_Result r=fvm_object_call(c,str,methods[m],1,&arg,&out);allowance=-1;
   assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);succeeded=r==FVM_OK;if(!succeeded)++failures;else if(!m)assert(fvm_gc_pin(c,out.object)==FVM_OK);
   string_work_clean(c);assert(fvm_gc_collect(c,0)==FVM_OK);
   if(!succeeded)assert(fvm_object_call(c,str,methods[m],1,&arg,&out)==FVM_OK);
   if(!m)number(c,out.object,"length",3);else assert(out.type==FVM_VALUE_NUMBER && out.number==(m==1?1:3));
  }
  assert(succeeded && failures);
 }
 fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
