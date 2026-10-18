/* Loaded SWF4-6 folds coordinate spelling without getters; SWF7-8 and host stay exact. */
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cassert>
static unsigned calls;
static FVM_Result trap(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{++calls;fvm_value_number(out,99);return FVM_OK;}
static void number(FVM_Context *c,FVM_Object *o,const char *key,double n)
{FVM_Value v;fvm_value_number(&v,n);assert(fvm_object_set(c,o,key,&v)==FVM_OK);}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned profile=0;profile<6;++profile)for(unsigned native=0;native<2;++native)for(unsigned mode=0;mode<5;++mode){
  FVM_Context *c=0;FVM_Object *clip=0,*point=0,*proto=0;FVM_Value v,out;FVM_Affine matrix={2,0,0,3,10,20};unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
  assert(fvm_context_create(rt,&c)==FVM_OK);if(profile){swf[3]=(unsigned char)(profile+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
  assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"clip",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK && fvm_movieclip_set_matrix(c,clip,&matrix)==FVM_OK && fvm_object_create(c,&point)==FVM_OK && fvm_gc_pin(c,point)==FVM_OK && fvm_object_create(c,&proto)==FVM_OK && fvm_gc_pin(c,proto)==FVM_OK);
  number(c,point,"Y",2);if(mode==0)number(c,point,"X",1);
  if(mode==1){number(c,proto,"X",1);point->prototype=proto;}
  if(mode==2)assert(fvm_object_define_accessor_rooted(c,point,"X",trap,0,0,0)==FVM_OK);
  if(mode==3){fvm_value_number(&v,1);assert(fvm_object_set_bytes(c,point,"X\0tail",6,&v)==FVM_OK);}
  if(mode==4)number(c,point,"x",1);
  fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=point;calls=0;FVM_GCRootFrame *roots=c->gc_frames;FVM_Result r=native?fvm_object_call(c,clip,"localToGlobal",1,&v,&out):fvm_exec_clip_point(c,clip,0,&v);
  bool folded=profile && profile<4 && (mode==0 || mode==4);assert(r==(profile?FVM_OK:FVM_INVALID_ARGUMENT) && !calls && c->gc_frames==roots && !c->gc_native_depth && !c->script_depth);assert(fvm_object_get(c,point,"Y",&out)==FVM_OK && out.number==(folded?26:2));if(mode==0){assert(fvm_object_get(c,point,"X",&out)==FVM_OK && out.number==(folded?12:1));assert(fvm_object_get(c,point,"x",&out)==FVM_OK && (profile && profile<4 ? out.type==FVM_VALUE_NUMBER && out.number==12 : out.type==FVM_VALUE_UNDEFINED));}
  assert(fvm_object_get(c,point,"y",&out)==FVM_OK && (profile && profile<4 ? out.type==FVM_VALUE_NUMBER && out.number==(folded?26:2) : out.type==FVM_VALUE_UNDEFINED));
  if(folded){assert((native?fvm_object_call(c,clip,"globalToLocal",1,&v,&out):fvm_exec_clip_point(c,clip,1,&v))==FVM_OK);assert(fvm_object_get(c,point,mode==4?"x":"X",&out)==FVM_OK && out.number==1 && fvm_object_get(c,point,"Y",&out)==FVM_OK && out.number==2);}
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
