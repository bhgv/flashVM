/* Character index callbacks share work; successful unit extraction costs one unit. */
#define FVM_STRING_WORK_FIXTURE_ONLY
#include "string_loaded_work.cpp"
#include "flashvm/exception.h"
static FVM_Object *inner;
static unsigned calls;
static bool fail;
static FVM_Result character_index(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{
 ++calls;assert(c->gc_native_depth && fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);
 if(fail){fvm_value_number(out,81);return fvm_exception_raise(c,out);}
 FVM_Value arg,nested;fvm_value_string(&arg,"z");FVM_Result r=fvm_object_call(c,inner,"indexOf",1,&arg,&nested);
 if(r==FVM_OK)fvm_value_number(out,0);return r;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);string_work_load(c,version);
  FVM_Object *str=string_work_make(c,"\xf0\x9f\x98\x80",4),*index=0;inner=string_work_make(c,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",40);
  assert(fvm_construct(c,"Object",0,0,&index)==FVM_OK && fvm_gc_pin(c,index)==FVM_OK && fvm_object_define_method_rooted(c,index,"valueOf",character_index,0,0)==FVM_OK);
  const char *names[]={"charAt","charCodeAt"};FVM_Value arg,out;
  for(unsigned method=0;method<2;++method){arg=object(index);calls=0;fail=false;rt->config.instruction_limit=16;
   assert(fvm_object_call(c,str,names[method],1,&arg,&out)==FVM_ERROR && calls==1 && c->avm1_steps==16 && out.type==FVM_VALUE_UNDEFINED);string_work_clean(c);
   rt->config.instruction_limit=0;calls=0;assert(fvm_object_call(c,str,names[method],1,&arg,&out)==FVM_OK && calls==1);string_work_clean(c);
   fail=true;calls=0;assert(fvm_object_call(c,str,names[method],1,&arg,&out)==FVM_EXCEPTION && calls==1 && fvm_exception_clear(c)==FVM_OK);fail=false;
   fvm_value_number(&arg,0);rt->config.instruction_limit=1;assert(fvm_object_call(c,str,names[method],1,&arg,&out)==FVM_OK && c->avm1_steps==1);
   if(method)assert(out.type==FVM_VALUE_NUMBER && out.number==0xd83d);else assert(fvm_value_string_size(&out)==3 && !std::memcmp(out.string,"\xed\xa0\xbd",3));
   c->avm1_execution_depth=1;c->avm1_steps=1;assert(fvm_object_call(c,str,names[method],1,&arg,&out)==FVM_ERROR && c->avm1_steps==1 && c->avm1_execution_depth==1);
   rt->config.instruction_limit=0;c->avm1_steps=std::numeric_limits<unsigned long>::max();assert(fvm_object_call(c,str,names[method],1,&arg,&out)==FVM_ERROR && c->avm1_steps==std::numeric_limits<unsigned long>::max());c->avm1_execution_depth=0;
   fvm_value_number(&arg,99);assert(fvm_object_call(c,str,names[method],1,&arg,&out)==FVM_OK && c->avm1_steps==0);string_work_clean(c);assert(fvm_gc_collect(c,0)==FVM_OK);
  }
  fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
