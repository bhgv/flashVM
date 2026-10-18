/* Diagnostic preparation/retry OOM cannot replace the original native exception or leak replay state. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/execution_mode.h"
#include "flashvm/exception.h"
static FVM_Result throws(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{fvm_value_number(out,111);return fvm_exception_raise(c,out);}
static void discard(FVM_Context *,const FVM_ExecutionEvent *,void *){}
int main()
{
 if(!fvm_jit_available())return 0;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *fn=0,*key=0,*method=0;FVM_Value arg,v,out;const char *params[]={"x"};
 const unsigned char body[]={0x96,3,0,0,'x',0,0x1c,0x96,5,0,7,2,0,0,0,0x0a,0x3e,0};const unsigned char empty[]={0};
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_function_create_avm1(c,body,sizeof(body),1,params,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK && fvm_object_create(c,&key)==FVM_OK && fvm_gc_pin(c,key)==FVM_OK && fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&method)==FVM_OK && fvm_function_set_body(c,method,throws,0,0)==FVM_OK);
 fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=method;assert(fvm_object_set(c,key,"valueOf",&v)==FVM_OK);arg=v;arg.object=key;
 assert(fvm_execution_set_mode(c,FVM_EXECUTION_JIT,FVM_EXEC_REPLAY_ON_EXCEPTION)==FVM_OK && fvm_execution_set_logger(c,discard,0)==FVM_OK);
 bool complete=false;unsigned failures=0,replay_oom=0;
 for(long boundary=0;boundary<3000 && !complete;++boundary)
 {
  FVM_GCRootFrame *roots=c->gc_frames;unsigned long previous=c->execution_mode.stats.replays;allowance=boundary;FVM_Result r=fvm_function_invoke(c,fn,0,1,&arg,&out);allowance=-1;
  assert(c->gc_frames==roots && !c->gc_native_depth && !c->script_depth && !c->avm1_execution_depth && !c->execution_mode.replay_depth);
  if(r==FVM_EXCEPTION){FVM_Value exception;assert(fvm_exception_get(c,&exception)==FVM_OK && exception.type==FVM_VALUE_NUMBER && exception.number==111);if(c->execution_mode.stats.replays>previous){if(c->execution_mode.stats.last_replay_status==FVM_OUT_OF_MEMORY)++replay_oom;else {assert(c->execution_mode.stats.last_replay_status==FVM_EXCEPTION);complete=true;}}assert(fvm_exception_clear(c)==FVM_OK);}
  else {assert(r==FVM_OUT_OF_MEMORY);++failures;}
  assert(fvm_gc_collect(c,0)==FVM_OK);
 }
 assert(complete && failures && replay_oom);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
