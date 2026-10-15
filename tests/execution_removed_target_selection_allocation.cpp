/* Failed loaded selection must not publish a target before removed-ancestry validation finishes. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
#include "flashvm/execution.h"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)
 {
  FVM_Context *c=0;FVM_Object *root=0;FVM_Value a[2],out,input;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
  fvm_value_string(a,"branch");fvm_value_number(a+1,1);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,a,&out)==FVM_OK);FVM_Object *branch=out.object;
  assert(fvm_object_set(c,root,"alias",&out)==FVM_OK);fvm_value_string(a,"nested");fvm_value_number(a+1,0);assert(fvm_object_call(c,branch,"createEmptyMovieClip",2,a,&out)==FVM_OK && fvm_exec_detach_clip(c,branch)==FVM_OK);
  assert(fvm_value_string(&input,"alias/nested")==FVM_OK);bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<2000 && !success;++boundary)
  {
   c->avm1_base_clip=root;c->avm1_target_clip=root;FVM_GCRootFrame *roots=c->gc_frames;
   allowance=boundary;FVM_Result r=fvm_exec_select_target(c,&input);allowance=-1;
   assert(c->gc_frames==roots && !c->script_depth);
   if(r==FVM_OK){assert(!c->avm1_target_clip);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && c->avm1_target_clip==root);++failures;}
   c->avm1_base_clip=0;c->avm1_target_clip=0;assert(fvm_gc_collect(c,0)==FVM_OK);
  }
  assert(success && failures);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
