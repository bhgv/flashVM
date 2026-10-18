/* Switching inside a native helper affects nested/new entries, not the active native frame. */
#include "flashvm/execution_mode.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cassert>
static FVM_Object *inner;
static FVM_Result switch_mode(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{
 FVM_Value value;FVM_ExecutionStats before,after;before.struct_size=after.struct_size=sizeof(before);
 assert(fvm_execution_get_stats(c,&before)==FVM_OK && fvm_execution_set_mode(c,FVM_EXECUTION_INTERPRETER,0)==FVM_OK && fvm_function_invoke(c,inner,0,0,0,&value)==FVM_OK && value.number==7 && fvm_execution_get_stats(c,&after)==FVM_OK && after.native_entries==before.native_entries && after.interpreter_entries==before.interpreter_entries+1);
 assert(fvm_execution_set_mode(c,FVM_EXECUTION_JIT,0)==FVM_OK && fvm_function_invoke(c,inner,0,0,0,&value)==FVM_OK && value.number==7 && fvm_execution_get_stats(c,&after)==FVM_OK && after.native_entries==before.native_entries+1);
 assert(fvm_gc_collect(c,0)==FVM_OK);fvm_value_number(out,40);return FVM_OK;
}
int main()
{
 if(!fvm_jit_available())return 0;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *fn=0,*key=0,*method=0;FVM_Value arg,v,out;const char *params[]={"x"};
 const unsigned char body[]={0x96,3,0,0,'x',0,0x1c,0x96,5,0,7,2,0,0,0,0x0a,0x3e,0};const unsigned char nested[]={0x96,5,0,7,7,0,0,0,0x3e,0};const unsigned char empty[]={0};
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_function_create_avm1(c,body,sizeof(body),1,params,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK && fvm_function_create_avm1(c,nested,sizeof(nested),0,0,0,&inner)==FVM_OK && fvm_gc_pin(c,inner)==FVM_OK && fvm_object_create(c,&key)==FVM_OK && fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&method)==FVM_OK && fvm_function_set_body(c,method,switch_mode,0,0)==FVM_OK);
 fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=method;assert(fvm_object_set(c,key,"valueOf",&v)==FVM_OK);arg=v;arg.object=key;
 assert(fvm_execution_set_mode(c,FVM_EXECUTION_JIT,0)==FVM_OK);FVM_GCRootFrame *roots=c->gc_frames;assert(fvm_function_invoke(c,fn,0,1,&arg,&out)==FVM_OK && out.number==42 && c->gc_frames==roots && !c->script_depth && !c->avm1_execution_depth);
 FVM_ExecutionStats stats;stats.struct_size=sizeof(stats);assert(fvm_execution_get_stats(c,&stats)==FVM_OK && stats.native_entries==2 && stats.interpreter_entries==1);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
