/* Prefix getter/callee GC keeps argument snapshots and the detached namespace receiver alive. */
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include <assert.h>
static FVM_Object *expected;static unsigned invoked;static int detach=1,failure,in_host;
static FVM_Result callee(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
 FVM_Value value;(void)raw;assert(frame->receiver==expected && frame->argc==1 && frame->argv[0].type==FVM_VALUE_OBJECT);
 assert(fvm_gc_collect(c,0)==(in_host?FVM_INVALID_ARGUMENT:FVM_OK));assert(fvm_object_get(c,frame->receiver,"alive",&value)==FVM_OK && value.number==42 && fvm_object_get(c,frame->argv[0].object,"alive",&value)==FVM_OK && value.number==17);++invoked;
 fvm_value_undefined(out);out->type=FVM_VALUE_OBJECT;out->object=frame->receiver;return FVM_OK;
}
static FVM_Result getter(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
 int removed=0;FVM_Value missing;(void)raw;if(detach){fvm_value_undefined(&missing);assert(fvm_set_global(c,"owner",&missing)==FVM_OK && fvm_object_delete(c,frame->receiver,"method",&removed)==FVM_OK && removed);}
 assert(fvm_gc_collect(c,0)==(in_host?FVM_INVALID_ARGUMENT:FVM_OK));if(failure){FVM_Value e;fvm_value_number(&e,77);return fvm_exception_raise(c,&e);}
 fvm_value_undefined(out);out->type=FVM_VALUE_OBJECT;out->object=frame->capture;return FVM_OK;
}
static void install(FVM_Context *c,FVM_Object *owner,FVM_Object *fn)
{
 const unsigned char code[]={0};FVM_Object *get;FVM_Value args[3],out;assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&get)==FVM_OK && fvm_function_set_body(c,get,getter,0,fn)==FVM_OK);
 fvm_value_string(args,"method");fvm_value_undefined(args+1);args[1].type=FVM_VALUE_OBJECT;args[1].object=get;fvm_value_undefined(args+2);args[2].type=FVM_VALUE_NULL;assert(fvm_object_call(c,owner,"addProperty",3,args,&out)==FVM_OK && out.boolean);
}
static FVM_Result host(FVM_Context *c,FVM_Object *self,unsigned n,const FVM_Value *args,FVM_Value *out,void *raw)
{ FVM_Value name;FVM_Result r;(void)self;(void)raw;fvm_value_string(&name,"owner:method");in_host=1;r=fvm_exec_call_function(c,0,0,0,0,0,&name,n,args,out);in_host=0;return r; }
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *owner,*arg,*fn,*host_fn;FVM_Value name,value,args[1],out;const unsigned char code[]={0};unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
 assert(fvm_construct(c,"Object",0,0,&owner)==FVM_OK && fvm_object_create(c,&arg)==FVM_OK);expected=owner;fvm_value_number(&value,42);assert(fvm_object_set(c,owner,"alive",&value)==FVM_OK);fvm_value_number(&value,17);assert(fvm_object_set(c,arg,"alive",&value)==FVM_OK);
 assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,callee,0,0)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK);install(c,owner,fn);
 fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=owner;assert(fvm_set_global(c,"owner",&value)==FVM_OK);fvm_value_undefined(args);args[0].type=FVM_VALUE_OBJECT;args[0].object=arg;fvm_value_string(&name,"owner:method");
 assert(fvm_exec_call_function(c,0,0,0,0,0,&name,1,args,&out)==FVM_OK && out.object==owner && invoked==1 && fvm_gc_pin(c,owner)==FVM_OK && fvm_gc_pin(c,arg)==FVM_OK);
 detach=0;assert(fvm_set_global(c,"owner",&value)==FVM_OK);install(c,owner,fn);failure=1;fvm_value_number(&out,99);assert(fvm_exec_call_function(c,0,0,0,0,0,&name,1,args,&out)==FVM_EXCEPTION && out.number==99 && invoked==1 && fvm_exception_clear(c)==FVM_OK);failure=0;
 assert(fvm_function_create_native(c,host,0,0,&host_fn)==FVM_OK && fvm_function_invoke(c,host_fn,0,1,args,&out)==FVM_OK && out.object==owner && invoked==2);assert(fvm_gc_collect(c,0)==FVM_OK);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
