/* Every construction allocation restores roots/barriers and preserves aliased error output. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)
 {
  bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<4000 && !success;++boundary)
  {
   FVM_Context *c=0;FVM_Object *root=0;FVM_Value a[2],out;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
   assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
   fvm_value_string(a,"source");fvm_value_number(a+1,1);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,a,&out)==FVM_OK);FVM_Object *clip=out.object;
   fvm_value_string(a,"copy");fvm_value_number(a+1,2);FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;FVM_Result r=fvm_exec_duplicate_clip(c,clip,a,a+1,0,a);allowance=-1;
   assert(c->gc_frames==roots && !c->gc_native_depth && !c->script_depth);
   if(r==FVM_OK){assert(a->type==FVM_VALUE_OBJECT && a->object!=clip);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && a->type==FVM_VALUE_STRING);++failures;}
   assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
  }
  assert(success && failures);
 }
 fvm_runtime_destroy(rt);return 0;
}
