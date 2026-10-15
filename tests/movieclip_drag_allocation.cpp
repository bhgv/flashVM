/* Script/C drag option allocation failures restore roots and preserve complete selection state. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(unsigned native=0;native<2;++native)
 {
  FVM_Context *c=0;FVM_Object *clip=0,*previous=0;FVM_Value args[5],out;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"clip",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK && fvm_movieclip_create(c,"previous",1,&previous)==FVM_OK && fvm_gc_pin(c,previous)==FVM_OK);
  fvm_value_boolean(args,1);fvm_value_number(args+1,0);fvm_value_number(args+2,0);fvm_value_number(args+3,10);fvm_value_number(args+4,10);bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<2000 && !success;++boundary)
  {
   assert(fvm_exec_drag_begin(c,previous,0,0)==FVM_OK);FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;FVM_Result r=native?fvm_object_call(c,clip,"startDrag",5,args,&out):fvm_exec_drag_values(c,clip,args,4,args+1);allowance=-1;
   assert(c->gc_frames==roots && !c->drag_updating && !c->gc_native_depth && !c->script_depth);
   if(r==FVM_OK){assert(c->drag_clip==clip);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && (c->drag_clip==clip || c->drag_clip==previous));++failures;}
   if(c->drag_clip==clip)assert(c->drag_lock_center && c->drag_constrained && c->drag_bounds.x_max==10 && c->drag_bounds.y_max==10);else assert(!c->drag_lock_center && !c->drag_constrained);
   assert(fvm_gc_collect(c,0)==FVM_OK);
  }
  assert(success && failures);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
