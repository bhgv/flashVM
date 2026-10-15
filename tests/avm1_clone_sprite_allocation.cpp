/* Every construction allocation restores roots/barriers and preserves aliased error output. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/execution.h"
#include "flashvm/avm1.h"
#include "../src/core/runtime.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(unsigned interpreter=0;interpreter<2;++interpreter)
 {
  bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<4000 && !success;++boundary)
  {
   FVM_Context *c=0;FVM_Object *root=0;FVM_Value a[2],out,source;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
   assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
   fvm_value_string(a,"source");fvm_value_number(a+1,1);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,a,&out)==FVM_OK);FVM_Object *clip=out.object;
   fvm_value_string(&source,"source");fvm_value_string(a,"copy");fvm_value_number(a+1,16386);FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;const unsigned char code[]={0x96,8,0,0,'s','o','u','r','c','e',0,0x96,6,0,0,'c','o','p','y',0,0x96,5,0,7,2,64,0,0,0x24,0};FVM_Result r=interpreter?fvm_avm1_execute_receiver(c,code,sizeof(code),root):fvm_exec_duplicate_target(c,root,&source,a,a+1,a);allowance=-1;
   assert(c->gc_frames==roots && !c->gc_native_depth && !c->script_depth && !c->avm1_target_active && !c->avm1_execution_depth);
   if(r==FVM_OK){assert(fvm_object_get(c,root,"copy",&out)==FVM_OK && out.type==FVM_VALUE_OBJECT && out.object!=clip);if(!interpreter)assert(a->object==out.object);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && a->type==FVM_VALUE_STRING);++failures;}
   assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
  }
  assert(success && failures);
 }
 fvm_runtime_destroy(rt);return 0;
}
