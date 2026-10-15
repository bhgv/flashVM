/* Inject every allocation boundary in budgeted interface declaration and verify cleanup. */
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
#define FVM_IMPLEMENTS_WORK_FIXTURE_ONLY
#include "implements_work.cpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);declaration_load(c,version);
  FVM_Object *subproto=0,*ifaceproto=0;FVM_Value sub=declaration_class(c,&subproto),iface=declaration_class(c,&ifaceproto),primitive;fvm_value_number(&primitive,1);
  allowance=0;FVM_Result zero=fvm_exec_implements(c,&sub,0,0),badctor=fvm_exec_implements(c,&primitive,1,&iface),legacy=version<7?fvm_exec_implements(c,&sub,1,&iface):FVM_OK;allowance=-1;
  assert(zero==FVM_OK && badctor==FVM_OK && legacy==FVM_OK && !subproto->interfaces_are_prototypes);declaration_clean(c);
  if(version>=7){fvm_detail::vector<FVM_Value> args;for(unsigned i=0;i<64;++i)args.push_back(iface);bool succeeded=false;unsigned failures=0;
   for(long boundary=0;boundary<4096 && !succeeded;++boundary){allowance=boundary;FVM_Result r=fvm_exec_implements(c,&sub,(unsigned)args.size(),args.begin());allowance=-1;
    assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);succeeded=r==FVM_OK;declaration_clean(c);
    if(!succeeded){++failures;assert(!subproto->interfaces_are_prototypes && subproto->implemented_interfaces.empty());}
    else assert(subproto->interfaces_are_prototypes && subproto->implemented_interfaces.size()==64);
    assert(fvm_gc_collect(c,0)==FVM_OK);
   }
   assert(succeeded && failures);
  }
  FVM_Context *other=0;FVM_Object *foreign=0;assert(fvm_context_create(rt,&other)==FVM_OK && fvm_object_create(other,&foreign)==FVM_OK);FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=foreign;
  allowance=0;FVM_Result r=fvm_exec_implements(c,&primitive,1,&value);allowance=-1;assert(r==FVM_INVALID_ARGUMENT);declaration_clean(c);
  fvm_context_destroy(other);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
