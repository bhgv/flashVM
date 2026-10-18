/* Local pointer queries restore native/root state at every allocation failure. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/execution.h"
#include "flashvm/input.h"
#include "../src/core/runtime.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(unsigned mode=0;mode<3;++mode)
 {
  FVM_Context *c=0;FVM_Object *clip=0;FVM_Value target,index,x,y;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK && fvm_input_set_pointer(c,3,4)==FVM_OK);
  fvm_value_undefined(&target);target.type=FVM_VALUE_OBJECT;target.object=clip;fvm_value_number(&index,21);bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<2000 && !success;++boundary)
  {
   fvm_value_number(&x,73);fvm_value_number(&y,74);FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;
   FVM_Result r=mode==0?fvm_exec_clip_pointer(c,clip,&x,&y):mode==1?fvm_object_get(c,clip,"_xmouse",&x):fvm_exec_display_property(c,0,&target,&index,0,&x);allowance=-1;
   assert(c->gc_frames==roots && !c->gc_native_depth && !c->script_depth);
   if(r==FVM_OK){assert(x.type==FVM_VALUE_NUMBER && x.number==(mode==2?4:3));if(!mode)assert(y.number==4);success=true;}
   else{assert(r==FVM_OUT_OF_MEMORY);if(!mode)assert(x.number==73 && y.number==74);++failures;}
   assert(fvm_gc_collect(c,0)==FVM_OK);
  }
  assert(success && failures);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
