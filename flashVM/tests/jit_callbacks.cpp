/* JIT helper callbacks collect, preserve activation roots, reject active release and propagate errors. */
#include "flashvm/jit.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cassert>
static FVM_JitCode *active;
static unsigned calls;
static bool fail;
static FVM_Result convert(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
 ++calls;assert(fvm_jit_destroy(active)==FVM_INVALID_ARGUMENT && fvm_gc_collect(c,0)==FVM_OK);
 FVM_Value v;assert(fvm_object_get(c,frame->receiver,"alive",&v)==FVM_OK && v.number==17);
 fvm_value_number(out,40);return fail?fvm_exception_raise(c,out):FVM_OK;
}
int main()
{
 if(!fvm_jit_available())return 0;FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 const unsigned char bytes[]={0x96,3,0,0,'x',0,0x1c,0x96,5,0,7,2,0,0,0,0x0a,0x3e,0};const char *params[]={"x"};const unsigned char empty[]={0};
 for(unsigned version=0;version<=8;++version){if(version && version<4)continue;
  FVM_Context *c=0;FVM_Object *fn=0,*object=0,*conversion=0;FVM_Value arg,v,out;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
  assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
  assert(fvm_standard_install(c)==FVM_OK && fvm_function_create_avm1(c,bytes,sizeof(bytes),1,params,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK && fvm_object_create(c,&object)==FVM_OK && fvm_gc_pin(c,object)==FVM_OK && fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&conversion)==FVM_OK && fvm_function_set_body(c,conversion,convert,0,0)==FVM_OK);
  fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=conversion;assert(fvm_object_set(c,object,"valueOf",&v)==FVM_OK);fvm_value_number(&v,17);assert(fvm_object_set(c,object,"alive",&v)==FVM_OK);
  assert(fvm_jit_compile(bytes,sizeof(bytes),&active)==FVM_OK && fvm_function_set_body(c,fn,fvm_jit_body,active,0)==FVM_OK);fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=object;
  for(unsigned failing=0;failing<2;++failing){fail=failing!=0;calls=0;FVM_GCRootFrame *roots=c->gc_frames;FVM_Result r=fvm_function_invoke(c,fn,0,1,&arg,&out);assert(calls==1 && c->gc_frames==roots && !c->gc_native_depth && !c->script_depth && !c->avm1_execution_depth);if(failing)assert(r==FVM_EXCEPTION && fvm_exception_clear(c)==FVM_OK);else assert(r==FVM_OK && out.number==42);}
  assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(active)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
