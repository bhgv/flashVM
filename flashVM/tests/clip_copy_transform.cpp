/* Local matrix/color copying is atomic, independent, alias-safe and transportable. */
#define FVM_TRANSFORM_FIXTURE_ONLY
#include "standard_transform.cpp"
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"
#include "flashvm/exception.h"
#include "flashvm/property.h"
static FVM_Result fail_get(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);FVM_Value value;fvm_value_number(&value,31);assert(fvm_object_set(c,(FVM_Object *)raw,"_y",&value)==FVM_OK);fvm_value_number(out,77);return fvm_exception_raise(c,out);}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned profile=0;profile<6;++profile)for(unsigned native=0;native<2;++native){
  FVM_Context *c=0,*other=0;FVM_Object *source=0,*dest=0;FVM_Value out,arg,plain;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);if(profile){swf[3]=(unsigned char)(profile+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
  assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"source",1,&source)==FVM_OK && fvm_gc_pin(c,source)==FVM_OK && fvm_movieclip_create(c,"dest",1,&dest)==FVM_OK && fvm_gc_pin(c,dest)==FVM_OK);
  FVM_Object *child=new_child(c,dest,"keptChild",3);set(c,dest,"keptMarker",73);
  double pose[]={-2,0.25,0.5,3,4,5},terms[]={0.5,1,2,1.5,11,12,13,-40};
  FVM_Object *transform=property(c,source,"transform");assign(c,transform,"matrix",construct(c,"flash.geom.Matrix",6,pose));assign(c,transform,"colorTransform",construct(c,"flash.geom.ColorTransform",8,terms));ref(&arg,transform);
  FVM_GCRootFrame *roots=c->gc_frames;assert((native?fvm_object_set(c,dest,"transform",&arg):fvm_exec_clip_copy_transform(c,dest,source))==FVM_OK && roots==c->gc_frames && !c->gc_native_depth);
  assert(fvm_object_get(c,dest,"keptChild",&out)==FVM_OK && out.object==child);near(num(c,dest,"keptMarker"),73);
  FVM_Object *target=property(c,dest,"transform");matrix(c,property(c,target,"matrix"),pose);color(c,property(c,target,"colorTransform"),terms);
  assert(fvm_exec_clip_copy_transform(c,dest,dest)==FVM_OK);set(c,source,"_x",99);matrix(c,property(c,target,"matrix"),pose);near(num(c,dest,"_alpha"),150);
  FVM_Object *object=0;assert(fvm_object_create(c,&object)==FVM_OK);ref(&plain,object);assert(fvm_object_set(c,dest,"transform",&plain)==FVM_OK && fvm_exec_clip_copy_transform(c,dest,object)==FVM_INVALID_ARGUMENT);matrix(c,property(c,target,"matrix"),pose);
  FVM_Channel *channel=0;ref(&arg,dest);assert(fvm_channel_create(&channel)==FVM_OK && fvm_channel_send_value(c,channel,&arg,0)==FVM_OK && fvm_channel_receive_value(other,channel,&out,0)==FVM_OK && fvm_gc_pin(other,out.object)==FVM_OK);fvm_channel_destroy(channel);
  FVM_Object *copy=out.object;assert(fvm_exec_clip_copy_transform(c,dest,copy)==FVM_INVALID_ARGUMENT);FVM_Object *copy_t=property(other,copy,"transform");matrix(other,property(other,copy_t,"matrix"),pose);ref(&arg,copy_t);assert(fvm_object_set(other,copy,"transform",&arg)==FVM_OK);
  int deleted=0;assert(fvm_object_delete(c,source,"_x",&deleted)==FVM_OK && deleted && fvm_object_define_accessor_rooted(c,source,"_x",fail_get,0,dest,dest)==FVM_OK);ref(&arg,transform);assert((native?fvm_object_set(c,dest,"transform",&arg):fvm_exec_clip_copy_transform(c,dest,source))==FVM_EXCEPTION && fvm_exception_clear(c)==FVM_OK);pose[5]=31;matrix(c,property(c,target,"matrix"),pose);color(c,property(c,target,"colorTransform"),terms);
  assert(c->gc_frames==roots && !c->gc_native_depth && !c->script_depth && fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(other);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
