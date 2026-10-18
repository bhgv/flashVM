/* Full drag Actions restore activation/root/update scopes at allocation failure boundaries. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/execution.h"
#include "flashvm/input.h"
#include "flashvm/avm1.h"
#include "../src/core/runtime.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 const unsigned char start[]={0x96,5,0,7,0,0,0,0,0x96,5,0,7,1,0,0,0,0x96,6,0,0,'c','l','i','p',0,0x27,0};const unsigned char end[]={0x28,0};
 for(unsigned version=4;version<=8;++version)for(unsigned stopping=0;stopping<2;++stopping)
 {
  FVM_Context *c=0;FVM_Object *root=0;FVM_Value a[2],out;FVM_Affine original={1,0,0,1,5,7},current;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);fvm_value_string(a,"clip");fvm_value_number(a+1,1);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,a,&out)==FVM_OK);FVM_Object *clip=out.object;bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<2000 && !success;++boundary)
  {
   assert(fvm_exec_drag_end(c)==FVM_OK && fvm_movieclip_set_matrix(c,clip,&original)==FVM_OK && fvm_input_set_pointer(c,0,0)==FVM_OK);if(stopping)assert(fvm_exec_drag_begin(c,clip,0,0)==FVM_OK && fvm_input_set_pointer(c,4,6)==FVM_OK);FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;
   FVM_Result r=fvm_avm1_execute_receiver(c,stopping?end:start,stopping?sizeof(end):sizeof(start),root);allowance=-1;
   assert(c->gc_frames==roots && !c->avm1_target_active && !c->avm1_execution_depth && !c->drag_updating && !c->gc_native_depth && !c->script_depth && fvm_movieclip_get_matrix(c,clip,&current)==FVM_OK);
   if(r==FVM_OK){assert(stopping?!c->drag_clip:c->drag_clip==clip);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && (!c->drag_clip || c->drag_clip==clip));++failures;}
   assert(current.x0==(stopping && !c->drag_clip?9:5) && current.y0==(stopping && !c->drag_clip?13:7) && fvm_gc_collect(c,0)==FVM_OK);
  }
  assert(success && failures);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
