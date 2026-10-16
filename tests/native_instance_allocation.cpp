/* Loaded native receiver allocation publishes no partial object across cold-wrapper OOM. */
#include "flashvm/standard.h"
#include "../src/core/string_internal.hpp"
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
#include "../src/standard/constructor_identity.hpp"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
static unsigned native_instance_calls;
static FVM_Result native_instance_trap(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{++native_instance_calls;fvm_value_number(out,91);return FVM_OK;}
int main()
{
 FVM_Config config;FVM_Runtime *runtime=0;assert(fvm_config_init(&config)==FVM_OK && fvm_runtime_create(&config,&runtime)==FVM_OK);
 for(unsigned version=4;version<=8;++version){bool failed=false,success=false;
  for(long boundary=0;boundary<512 && !success;++boundary){
   FVM_Context *c=0;assert(fvm_context_create(runtime,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
   assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
   FVM_Value klass,value;assert(fvm_get_global(c,"Number",&klass)==FVM_OK);int removed=0;
   assert(fvm_object_set_property_flags(c,klass.object,"prototype",0,FVM_PROPERTY_VALID_FLAGS)==FVM_OK && fvm_object_delete(c,klass.object,"prototype",&removed)==FVM_OK && removed);
   assert(fvm_object_define_method_rooted(c,klass.object,"prototype",native_instance_trap,0,0)==FVM_OK);
   FVM_Object *instance=(FVM_Object *)1;native_instance_calls=0;allowance=boundary;
   FVM_Result status=fvm_function_allocate_instance(c,klass.object,FVM_RESOURCE_NUMBER,&instance);allowance=-1;
   assert(!native_instance_calls && !c->gc_frames && !c->gc_native_depth && !c->exception_pending);
   if(status==FVM_OUT_OF_MEMORY){failed=true;assert(!instance && fvm_gc_collect(c,0)==FVM_OK);assert(fvm_function_allocate_instance(c,klass.object,FVM_RESOURCE_NUMBER,&instance)==FVM_OK);}
   else{assert(status==FVM_OK);success=true;}
   assert(instance && fvm_gc_pin(c,instance)==FVM_OK && fvm_object_get(c,klass.object,"prototype",&value)==FVM_OK && instance->prototype==value.object);
   assert(fvm_object_get(c,instance,"__constructor__",&value)==FVM_OK && value.object==klass.object);
   int own=0;assert(fvm_object_has_own(c,instance,"constructor",&own)==FVM_OK && !!own==(version<7));
   assert(fvm_gc_collect(c,0)==FVM_OK && !native_instance_calls);fvm_context_destroy(c);
  }assert(failed && success);
 }
 fvm_runtime_destroy(runtime);return 0;
}
