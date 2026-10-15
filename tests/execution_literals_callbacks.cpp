/* Name callbacks may collect and overwrite caller buffers without losing the entry snapshot. */
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cassert>
static FVM_ExecutionLiteralEntry *inputs;
static FVM_Object *payload;
static unsigned calls;
static bool fail;
static FVM_Result name(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{
 ++calls;fvm_value_string(&inputs[1].name,"mutated");fvm_value_number(&inputs[1].value,999);
 assert(fvm_gc_collect(c,0)==FVM_OK);FVM_Value v;assert(fvm_object_get(c,payload,"alive",&v)==FVM_OK && v.number==17);
 fvm_value_string(out,"first");return fail?fvm_exception_raise(c,out):FVM_OK;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned profile=0;profile<6;++profile)for(unsigned failing=0;failing<2;++failing)
 {
  FVM_Context *c=0;FVM_Object *key=0,*fn=0;FVM_Value v,out;FVM_ExecutionLiteralEntry entries[2];unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};const unsigned char code[]={0};
  assert(fvm_context_create(rt,&c)==FVM_OK);if(profile){swf[3]=(unsigned char)(profile+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
  assert(fvm_standard_install(c)==FVM_OK && fvm_object_create(c,&key)==FVM_OK && fvm_object_create(c,&payload)==FVM_OK && fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,name,0,0)==FVM_OK);
  fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=fn;assert(fvm_object_set(c,key,"toString",&v)==FVM_OK);fvm_value_number(&v,17);assert(fvm_object_set(c,payload,"alive",&v)==FVM_OK);
  fvm_value_undefined(&entries[0].name);entries[0].name.type=FVM_VALUE_OBJECT;entries[0].name.object=key;fvm_value_number(&entries[0].value,1);fvm_value_string(&entries[1].name,"second");fvm_value_undefined(&entries[1].value);entries[1].value.type=FVM_VALUE_OBJECT;entries[1].value.object=payload;
  inputs=entries;calls=0;fail=failing!=0;fvm_value_number(&out,99);FVM_GCRootFrame *roots=c->gc_frames;
  FVM_Result r=fvm_exec_literal_object(c,2,entries,&out);assert(calls==1 && c->gc_frames==roots && !c->gc_native_depth && !c->script_depth);
  if(failing)assert(r==FVM_EXCEPTION && out.type==FVM_VALUE_NUMBER && out.number==99 && fvm_exception_clear(c)==FVM_OK);
  else{assert(r==FVM_OK && out.type==FVM_VALUE_OBJECT && fvm_object_get(c,out.object,"second",&v)==FVM_OK && v.type==FVM_VALUE_OBJECT && v.object==payload);assert(fvm_object_get(c,out.object,"mutated",&v)==FVM_OK && v.type==FVM_VALUE_UNDEFINED);}
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
