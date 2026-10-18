/* C89 named calls retain qualified/default/With receiver identity and ordered counted arguments. */
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/standard.h"
#include <assert.h>
#include <string.h>
static FVM_Object *expected;
static unsigned calls;
static FVM_Result body(FVM_Context *c,FVM_Object *self,unsigned n,const FVM_Value *args,FVM_Value *out,void *raw)
{ (void)c;(void)raw;assert(self==expected && n==2 && args[0].number==17 && args[1].type==FVM_VALUE_STRING && fvm_value_string_size(args+1)==3 && !memcmp(args[1].string,"a\0b",3));++calls;fvm_value_number(out,42);return FVM_OK; }
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned version;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(version=0;version<=8;++version){FVM_Context *c;FVM_Object *owner,*fallback,*fn,*scope;FVM_Value name,value,args[2],out;FVM_Result r;
 unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};if(version && version<4)continue;swf[3]=(unsigned char)version;
 assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK && fvm_object_create(c,&owner)==FVM_OK && fvm_object_create(c,&fallback)==FVM_OK && fvm_function_create_native(c,body,0,0,&fn)==FVM_OK);
 fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=owner;assert(fvm_set_global(c,"owner",&value)==FVM_OK && fvm_exec_with_scope(c,0,&value,&scope)==FVM_OK);value.object=fn;assert(fvm_object_set(c,owner,"method",&value)==FVM_OK && fvm_object_set_bytes(c,owner,"m\0x",3,&value)==FVM_OK && fvm_set_global(c,"plain",&value)==FVM_OK && fvm_set_global(c,"literal.name",&value)==FVM_OK);
 fvm_value_number(args,17);fvm_value_string_bytes(args+1,"a\0b",3);expected=owner;
 if(version){fvm_value_string(&name,"owner:method");assert(fvm_exec_call_function(c,0,0,0,fallback,0,&name,2,args,&name)==FVM_OK && name.number==42);fvm_value_string_bytes(&name,"owner:m\0x",9);assert(fvm_exec_call_function(c,0,0,0,fallback,0,&name,2,args,&out)==FVM_OK && out.number==42);}
 fvm_value_string(&name,"method");assert(fvm_exec_call_function(c,scope,0,0,fallback,1,&name,2,args,&out)==FVM_OK && out.number==42);
 expected=fallback;fvm_value_string(&name,"plain");assert(fvm_exec_call_function(c,0,0,0,fallback,0,&name,2,args,&out)==FVM_OK && out.number==42);
 if(version>=5){fvm_value_string(&name,"owner/method");assert(fvm_exec_call_function(c,0,0,0,fallback,0,&name,2,args,&out)==FVM_OK && out.number==42);}
 if(!version){fvm_value_string(&name,"literal.name");assert(fvm_exec_call_function(c,0,0,0,fallback,0,&name,2,args,&out)==FVM_OK && out.number==42);}
 fvm_value_number(&value,7);assert(fvm_set_global(c,"bad",&value)==FVM_OK);fvm_value_string(&name,"bad");fvm_value_number(&out,99);r=fvm_exec_call_function(c,0,0,0,0,0,&name,0,0,&out);assert(version?(r==FVM_OK && out.type==FVM_VALUE_UNDEFINED):(r==FVM_UNSUPPORTED && out.number==99));
 fvm_value_string(&name,"absent");fvm_value_number(&out,99);r=fvm_exec_call_function(c,0,0,0,0,0,&name,0,0,&out);assert(version?(r==FVM_OK && out.type==FVM_VALUE_UNDEFINED):(r==FVM_UNSUPPORTED && out.number==99));
 fvm_value_string(&name,"Number");assert(fvm_exec_call_function(c,0,0,0,0,0,&name,1,args,&out)==FVM_OK && out.type==FVM_VALUE_NUMBER && out.number==17);
 fvm_value_number(&name,1);fvm_value_number(&out,99);assert(fvm_exec_call_function(c,0,0,0,0,0,&name,0,0,&out)==FVM_INVALID_ARGUMENT && out.number==99);
 fvm_context_destroy(c);}
 assert(calls>0);fvm_runtime_destroy(rt);return 0;
}
