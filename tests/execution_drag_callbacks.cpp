/* Matrix callbacks may cancel/replace a drag; stale work never publishes a transform. */
#include "flashvm/execution.h"
#include "flashvm/input.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cassert>
static unsigned mode;
static FVM_Object *replacement;
static FVM_Result read_x(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{
 assert(c->drag_updating && fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT && fvm_exec_drag_update(c)==FVM_INVALID_ARGUMENT);fvm_value_number(out,5);
 if(mode==0)return fvm_exception_raise(c,out);if(mode==1)assert(fvm_exec_drag_end(c)==FVM_OK);if(mode==2)assert(fvm_exec_drag_begin(c,replacement,1,0)==FVM_OK);return FVM_OK;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(mode=0;mode<4;++mode)
 {
  FVM_Context *c=0;FVM_Object *clip=0;FVM_Value out;int removed=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"clip",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK && fvm_movieclip_create(c,"replacement",1,&replacement)==FVM_OK && fvm_gc_pin(c,replacement)==FVM_OK);
  assert(fvm_object_delete(c,clip,"_x",&removed)==FVM_OK && removed && fvm_object_define_accessor_rooted(c,clip,"_x",read_x,0,0,0)==FVM_OK && fvm_input_set_pointer(c,10,20)==FVM_OK && fvm_exec_drag_begin(c,clip,1,0)==FVM_OK);FVM_GCRootFrame *roots=c->gc_frames;FVM_Result r=fvm_exec_drag_update(c);
  assert(c->gc_frames==roots && !c->drag_updating && !c->gc_native_depth && !c->script_depth);
  if(mode==0){assert(r==FVM_EXCEPTION && c->drag_clip==clip && clip->properties.find("_x")==clip->properties.end() && fvm_exception_clear(c)==FVM_OK);mode=3;assert(fvm_exec_drag_update(c)==FVM_OK);mode=0;}
  else if(mode==1)assert(r==FVM_OK && !c->drag_clip && clip->properties.find("_x")==clip->properties.end());
  else if(mode==2)assert(r==FVM_OK && c->drag_clip==replacement && clip->properties.find("_x")==clip->properties.end());
  else assert(r==FVM_OK && c->drag_clip==clip && clip->properties.find("_x")->second.number==10);
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
