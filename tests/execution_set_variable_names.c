/* C89 loaded SetVariable name conversion, GC roots and exception propagation. */
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include <assert.h>
static unsigned calls;
static int fail;
static FVM_Result convert(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
 FVM_Value v;(void)raw;++calls;assert(fvm_gc_collect(c,0)==FVM_OK && fvm_object_get(c,frame->receiver,"alive",&v)==FVM_OK && v.number==17);if(fail){fvm_value_number(&v,77);return fvm_exception_raise(c,&v);}return fvm_value_string_bytes(out,"name\0x",6);
}
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned version;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(version=0;version<=8;++version){FVM_Context *c;FVM_Object *name,*fn,*target,*assigned;FVM_Value key,value,out;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};const unsigned char code[]={0};if(version && version<4)continue;assert(fvm_context_create(rt,&c)==FVM_OK);if(version){swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}assert(fvm_standard_install(c)==FVM_OK);
 fvm_value_number(&key,17);fvm_value_number(&value,42);assert(fvm_exec_variable_set(c,0,0,0,&key,&value)==(version?FVM_OK:FVM_INVALID_ARGUMENT));if(version)assert(fvm_get_global(c,"17",&out)==FVM_OK && out.number==42);
 assert(fvm_object_create(c,&name)==FVM_OK && fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,convert,0,0)==FVM_OK);fvm_value_number(&value,17);assert(fvm_object_set(c,name,"alive",&value)==FVM_OK);value.type=FVM_VALUE_OBJECT;value.object=fn;assert(fvm_object_set(c,name,"toString",&value)==FVM_OK);key.type=FVM_VALUE_OBJECT;key.object=name;assert(fvm_gc_pin(c,name)==FVM_OK && fvm_object_create(c,&target)==FVM_OK && fvm_gc_pin(c,target)==FVM_OK && fvm_object_create(c,&assigned)==FVM_OK);value.type=FVM_VALUE_OBJECT;value.object=assigned;
 assert(fvm_exec_variable_set(c,0,target,0,&key,&value)==(version?FVM_OK:FVM_INVALID_ARGUMENT));if(version){assert(fvm_object_get_bytes(c,target,"name\0x",6,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT && out.object==assigned);fail=1;fvm_value_number(&value,99);assert(fvm_exec_variable_set(c,0,target,0,&key,&value)==FVM_EXCEPTION && fvm_exception_clear(c)==FVM_OK);fail=0;assert(fvm_object_get_bytes(c,target,"name\0x",6,&out)==FVM_OK && out.object==assigned);}fvm_context_destroy(c);}
 assert(calls==10);fvm_runtime_destroy(rt);return 0;
}
