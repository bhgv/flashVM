/* Inject every allocation boundary in counted field edits and verify cleanup. */
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
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
#ifdef FVM_HAS_FONTS
 for(unsigned method=0;method<2;++method){bool succeeded=false;unsigned failures=0;
  for(long boundary=0;boundary<4096 && !succeeded;++boundary){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);FVM_Object *field=counted_field(c);FVM_Value value,args[3],out;
   assert(fvm_value_string_bytes(&value,"A\0B",3)==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);fvm_value_number(args,1);fvm_value_number(args+1,2);assert(fvm_value_string_bytes(args+2,"X\0Y",3)==FVM_OK);
   allowance=boundary;FVM_Result r=method?fvm_object_call(c,field,"replaceText",3,args,&out):fvm_object_set(c,field,"text",args+2);allowance=-1;
   assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);succeeded=r==FVM_OK;if(!succeeded)++failures;
   assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);counted_text(c,field,succeeded?(method?"AX\0YB":"X\0Y"):"A\0B",succeeded&&method?5:3);
   if(!succeeded){assert((method?fvm_object_call(c,field,"replaceText",3,args,&out):fvm_object_set(c,field,"text",args+2))==FVM_OK);counted_text(c,field,method?"AX\0YB":"X\0Y",method?5:3);}
   fvm_context_destroy(c);
  }
  assert(succeeded && failures);
 }
#endif
 fvm_runtime_destroy(rt);
}
