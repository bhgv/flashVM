/* Radix coercion cannot replace captured numeric data; native GC and exception policy stay intact. */
#define FVM_LOADED_RADIX_FIXTURE_ONLY
#include "number_loaded_radix.cpp"
#include "flashvm/exception.h"
static FVM_Object *source;
static unsigned calls,phase;
static FVM_Result radix_change(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{
 ++calls;assert(c->gc_native_depth && fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);source->resource_result.number=7.5;
 if(phase==1){fvm_value_number(out,81);return fvm_exception_raise(c,out);}if(phase==2)return FVM_ERROR;
 fvm_value_number(out,3);return FVM_OK;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(phase=0;phase<3;++phase){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);radix_loaded(c,version);source=radix_box(c,42.9);assert(fvm_gc_pin(c,source)==FVM_OK);
  FVM_Object *radix=0;assert(fvm_construct(c,"Object",0,0,&radix)==FVM_OK && fvm_object_define_method_rooted(c,radix,"valueOf",radix_change,0,0)==FVM_OK);FVM_Value arg=object(radix),out;calls=0;
  FVM_Result r=fvm_object_call(c,source,"toString",1,&arg,&out);assert(calls==1 && source->resource_result.number==7.5);
  if(!phase)assert(r==FVM_OK && !std::strcmp(out.string,"1120"));else if(phase==1)assert(r==FVM_EXCEPTION && fvm_exception_clear(c)==FVM_OK);else assert(r==FVM_ERROR);
  assert(!c->gc_frames && !c->gc_native_depth && !c->primitive_depth && !c->string_depth);assert(fvm_gc_collect(c,0)==FVM_OK);radix_expect(c,7.5,3,"21");fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
