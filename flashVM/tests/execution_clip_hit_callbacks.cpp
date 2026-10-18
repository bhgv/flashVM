/* Intrinsic geometry getters retain native barriers and restore scopes on script errors. */
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cassert>
static unsigned calls;
static bool fail_getter;
static FVM_Result read_x(FVM_Context *c,FVM_Object *,unsigned int,const FVM_Value *,FVM_Value *out,void *)
{++calls;assert(c->gc_native_depth && fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);fvm_value_number(out,0);if(fail_getter)return fvm_exception_raise(c,out);return FVM_OK;}
static void coordinate(FVM_Context *c,FVM_Object *clip,const char *method,double x,double y)
{FVM_Value a[2],out;fvm_value_number(a,x);fvm_value_number(a+1,y);assert(fvm_object_call(c,clip,method,2,a,&out)==FVM_OK);}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned profile=0;profile<6;++profile)
 {
  FVM_Context *c=0;FVM_Object *clip=0;FVM_Value a,out;int removed=0;
  unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
  assert(fvm_context_create(rt,&c)==FVM_OK);if(profile){swf[3]=(unsigned char)(profile+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
  assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);
  fvm_value_number(&a,0xff0000);assert(fvm_object_call(c,clip,"beginFill",1,&a,&out)==FVM_OK);coordinate(c,clip,"moveTo",0,0);coordinate(c,clip,"lineTo",10,0);coordinate(c,clip,"lineTo",0,10);assert(fvm_object_call(c,clip,"endFill",0,0,&out)==FVM_OK);
  assert(fvm_object_delete(c,clip,"_x",&removed)==FVM_OK && removed && fvm_object_define_accessor_rooted(c,clip,"_x",read_x,0,0,0)==FVM_OK);
  for(unsigned mode=0;mode<3;++mode)for(unsigned failing=0;failing<2;++failing)
  {
   calls=0;fail_getter=failing!=0;fvm_value_number(&out,73);FVM_GCRootFrame *roots=c->gc_frames;
   FVM_Result r=mode==2?fvm_exec_clip_hit_object(c,clip,clip,&out):fvm_exec_clip_hit_point(c,clip,2,2,mode,&out);
   assert(calls && c->gc_frames==roots && !c->gc_native_depth && !c->script_depth);
   if(failing)assert(r==FVM_EXCEPTION && out.type==FVM_VALUE_NUMBER && out.number==73 && fvm_exception_clear(c)==FVM_OK);
   else assert(r==FVM_OK && out.type==FVM_VALUE_BOOLEAN && out.boolean);
   assert(fvm_gc_collect(c,0)==FVM_OK);
  }
  fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
