/* C89 intrinsic root-lock access is versioned, alias-safe and independent of activation state. */
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/property.h"
#include <assert.h>
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned profile;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(profile=0;profile<6;++profile)
 {
  FVM_Context *c,*other;FVM_Object *clip,*foreign,*plain;FVM_Value value,out,ctor,proto;unsigned flags;int removed;
  unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);if(profile){swf[3]=(unsigned char)(profile+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
  assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK && fvm_object_create(c,&plain)==FVM_OK && fvm_object_create(other,&foreign)==FVM_OK);
  assert(fvm_exec_clip_lock_root(c,0,clip,0,&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN && !out.boolean);
  fvm_value_number(&out,73);fvm_value_undefined(&value);
  assert(fvm_exec_clip_lock_root(0,0,clip,0,&out)==FVM_INVALID_ARGUMENT && fvm_exec_clip_lock_root(c,0,plain,0,&out)==FVM_INVALID_ARGUMENT && fvm_exec_clip_lock_root(c,0,foreign,0,&out)==FVM_INVALID_ARGUMENT && fvm_exec_clip_lock_root(c,1,clip,0,&out)==FVM_INVALID_ARGUMENT && out.number==73);
  assert(fvm_value_string(&value,"0")==FVM_OK && fvm_exec_clip_lock_root(c,1,clip,&value,&value)==FVM_OK && value.type==FVM_VALUE_BOOLEAN && !!value.boolean==(!profile || profile+3>=7));
  assert(fvm_object_get(c,clip,"_lockroot",&out)==FVM_OK && out.boolean==value.boolean);
  fvm_value_boolean(&value,1);assert(fvm_object_set(c,clip,"_lockroot",&value)==FVM_OK && fvm_exec_clip_lock_root(c,0,clip,0,&out)==FVM_OK && out.boolean);
  assert(fvm_get_global(c,"MovieClip",&ctor)==FVM_OK && fvm_object_get(c,ctor.object,"prototype",&proto)==FVM_OK && fvm_object_get(c,proto.object,"_lockroot",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
  assert(fvm_object_property_flags(c,proto.object,"_lockroot",&flags)==FVM_OK && (flags&(FVM_PROPERTY_DONT_ENUM|FVM_PROPERTY_DONT_DELETE))==(FVM_PROPERTY_DONT_ENUM|FVM_PROPERTY_DONT_DELETE));
  assert(fvm_object_delete(c,proto.object,"_lockroot",&removed)==FVM_OK && !removed);
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(other);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
