/* Every failed root-lock assignment preserves the previous flag and aliased output. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
#include "flashvm/execution.h"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(unsigned previous=0;previous<2;++previous)
 {
  FVM_Context *c=0;FVM_Object *clip=0;FVM_Value input,out;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);
  if(previous){fvm_value_boolean(&input,1);assert(fvm_exec_clip_lock_root(c,1,clip,&input,&out)==FVM_OK);}
  bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<2000 && !success;++boundary)
  {
   assert(fvm_value_string(&input,previous?"":"1")==FVM_OK);FVM_GCRootFrame *roots=c->gc_frames;
   allowance=boundary;FVM_Result r=fvm_exec_clip_lock_root(c,1,clip,&input,&input);allowance=-1;
   assert(c->gc_frames==roots && fvm_exec_clip_lock_root(c,0,clip,0,&out)==FVM_OK);
   if(r==FVM_OK){assert(input.type==FVM_VALUE_BOOLEAN && !!out.boolean==!previous);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && input.type==FVM_VALUE_STRING && !!out.boolean==!!previous);++failures;}
   assert(fvm_gc_collect(c,0)==FVM_OK);
  }
  assert(success && failures);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
