/* String receiver/limit/separator callbacks share nested work and retain native GC policy. */
#define FVM_STRING_WORK_FIXTURE_ONLY
#include "string_loaded_work.cpp"
#include "flashvm/exception.h"
static FVM_Object *inner;
static unsigned order[3],calls,failure;
static FVM_Result work_callback(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{
 unsigned id=*(unsigned *)raw;assert(calls<3);order[calls++]=id;assert(c->gc_native_depth && fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);
 if(id==1){FVM_Value arg,nested;fvm_value_string(&arg,"z");FVM_Result r=fvm_object_call(c,inner,"indexOf",1,&arg,&nested);if(r!=FVM_OK)return r;}
 if(failure==id){fvm_value_number(out,81);return fvm_exception_raise(c,out);}
 if(id==2){fvm_value_number(out,3);return FVM_OK;}return fvm_value_string(out,id==1?"a,b,c":",");
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);string_work_load(c,version);
  FVM_Object *str=string_work_make(c,"",0),*receiver=0,*limit=0,*separator=0;inner=string_work_make(c,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",40);
  assert(fvm_construct(c,"Object",0,0,&receiver)==FVM_OK && fvm_construct(c,"Object",0,0,&limit)==FVM_OK && fvm_construct(c,"Object",0,0,&separator)==FVM_OK);
  unsigned ids[]={1,2,3};assert(fvm_object_define_method_rooted(c,receiver,"toString",work_callback,ids,0)==FVM_OK && fvm_object_define_method_rooted(c,limit,"valueOf",work_callback,ids+1,0)==FVM_OK && fvm_object_define_method_rooted(c,separator,"toString",work_callback,ids+2,0)==FVM_OK);
  assert(fvm_gc_pin(c,receiver)==FVM_OK && fvm_gc_pin(c,limit)==FVM_OK && fvm_gc_pin(c,separator)==FVM_OK);
  FVM_Value args[2],out;args[0]=object(separator);args[1]=object(limit);unsigned long old=rt->config.instruction_limit;rt->config.instruction_limit=16;calls=0;failure=0;
  assert(borrow(c,str,"split",receiver,2,args,&out)==FVM_ERROR && calls==1 && c->avm1_steps==16);string_work_clean(c);rt->config.instruction_limit=0;
  for(failure=0;failure<4;++failure){calls=0;FVM_Result r=borrow(c,str,"split",receiver,2,args,&out);assert(calls==(failure?failure:3));for(unsigned i=0;i<calls;++i)assert(order[i]==i+1);
   if(failure)assert(r==FVM_EXCEPTION && fvm_exception_clear(c)==FVM_OK);else{assert(r==FVM_OK);number(c,out.object,"length",3);}string_work_clean(c);
  }
  rt->config.instruction_limit=old;assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
