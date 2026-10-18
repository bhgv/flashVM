/* Native exceptions retry once with fresh interpreter activation, log steps and preserve original throw. */
#include "flashvm/execution_mode.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cassert>
static unsigned calls,events[5];
static bool throw_again;
static FVM_Object *original;
static FVM_Result value(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{
 ++calls;
 if(calls==1){assert(fvm_object_create(c,&original)==FVM_OK);FVM_Value marker;fvm_value_number(&marker,111);assert(fvm_object_set(c,original,"marker",&marker)==FVM_OK);fvm_value_undefined(out);out->type=FVM_VALUE_OBJECT;out->object=original;return fvm_exception_raise(c,out);}
 assert(calls==2 && c->execution_mode.replay_depth==1 && fvm_gc_collect(c,0)==FVM_OK);FVM_Value marker;assert(fvm_object_get(c,original,"marker",&marker)==FVM_OK && marker.number==111);
 assert(fvm_execution_set_mode(c,FVM_EXECUTION_JIT,FVM_EXEC_REPLAY_ON_EXCEPTION)==FVM_OK);fvm_value_number(out,throw_again?222:40);return throw_again?fvm_exception_raise(c,out):FVM_OK;
}
static void log_event(FVM_Context *c,const FVM_ExecutionEvent *event,void *)
{
 assert(event->kind>=FVM_EXEC_EVENT_NATIVE_EXCEPTION && event->kind<=FVM_EXEC_EVENT_REPLAY_END);++events[event->kind];FVM_ExecutionStats stats;stats.struct_size=sizeof(stats);assert(fvm_execution_get_stats(c,&stats)==FVM_OK && stats.native_entries==1);
}
int main()
{
 if(!fvm_jit_available())return 0;FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 const unsigned char body[]={0x96,3,0,0,'x',0,0x1c,0x96,5,0,7,2,0,0,0,0x0a,0x3e,0};const unsigned char empty[]={0};const char *params[]={"x"};
 for(unsigned again=0;again<2;++again)
 {
  FVM_Context *c=0;FVM_Object *fn=0,*object=0,*method=0;FVM_Value arg,v,out,exception;
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_function_create_avm1(c,body,sizeof(body),1,params,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK && fvm_object_create(c,&object)==FVM_OK && fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&method)==FVM_OK && fvm_function_set_body(c,method,value,0,0)==FVM_OK);
  fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=method;assert(fvm_object_set(c,object,"valueOf",&v)==FVM_OK);arg=v;arg.object=object;
  assert(fvm_execution_set_logger(c,log_event,0)==FVM_OK && fvm_execution_set_mode(c,FVM_EXECUTION_JIT,FVM_EXEC_REPLAY_ON_EXCEPTION)==FVM_OK);calls=0;throw_again=again!=0;for(unsigned i=0;i<5;++i)events[i]=0;
  FVM_GCRootFrame *roots=c->gc_frames;assert(fvm_function_invoke(c,fn,0,1,&arg,&out)==FVM_EXCEPTION && calls==2 && c->gc_frames==roots && !c->script_depth && !c->avm1_execution_depth && !c->execution_mode.replay_depth && fvm_exception_get(c,&exception)==FVM_OK && exception.type==FVM_VALUE_OBJECT && exception.object==original);
  FVM_ExecutionStats stats;stats.struct_size=sizeof(stats);assert(fvm_execution_get_stats(c,&stats)==FVM_OK && stats.replays==1 && stats.replay_steps>0 && stats.last_replay_status==(again?FVM_EXCEPTION:FVM_OK) && stats.native_entries==1 && events[FVM_EXEC_EVENT_NATIVE_EXCEPTION]==1 && events[FVM_EXEC_EVENT_REPLAY_BEGIN]==1 && events[FVM_EXEC_EVENT_REPLAY_END]==1 && events[FVM_EXEC_EVENT_INTERPRETER_STEP]==stats.replay_steps);
  assert(fvm_exception_clear(c)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
