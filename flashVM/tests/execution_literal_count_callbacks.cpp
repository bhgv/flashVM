/* Direct C count conversion roots an otherwise unreachable receiver through collecting coercion. */
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cassert>
static unsigned calls;
static bool fail;
static FVM_Result count(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
 ++calls;assert(fvm_gc_collect(c,0)==FVM_OK);FVM_Value v;assert(fvm_object_get(c,frame->receiver,"alive",&v)==FVM_OK && v.number==17);
 fvm_value_number(out,1.9);return fail?fvm_exception_raise(c,out):FVM_OK;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(unsigned failing=0;failing<2;++failing)
 {
  FVM_Context *c=0;FVM_Object *object=0,*fn=0;FVM_Value v,input;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};const unsigned char code[]={0};swf[3]=(unsigned char)version;
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_object_create(c,&object)==FVM_OK && fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,count,0,0)==FVM_OK);
  fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=fn;assert(fvm_object_set(c,object,"valueOf",&v)==FVM_OK);fvm_value_number(&v,17);assert(fvm_object_set(c,object,"alive",&v)==FVM_OK);
  fvm_value_undefined(&input);input.type=FVM_VALUE_OBJECT;input.object=object;FVM_GCRootFrame *roots=c->gc_frames;calls=0;fail=failing!=0;
  FVM_Result r=fvm_exec_literal_count(c,&input,&input);assert(calls==1 && c->gc_frames==roots && !c->gc_native_depth && !c->script_depth);
  if(failing)assert(r==FVM_EXCEPTION && input.type==FVM_VALUE_OBJECT && input.object==object && fvm_exception_clear(c)==FVM_OK);else assert(r==FVM_OK && input.type==FVM_VALUE_NUMBER && input.number==1);
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
