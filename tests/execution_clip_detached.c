/* C89 intrinsic marker separates new roots, detached wrappers and depth replacements. */
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
  FVM_Context *c,*other;FVM_Object *root,*old,*fresh,*plain,*foreign;FVM_Value args[2],v,out;
  unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
  if(profile){swf[3]=(unsigned char)(profile+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
  assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
  assert(fvm_exec_clip_detached(c,root,&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN && !out.boolean);
  assert(fvm_object_create(c,&plain)==FVM_OK && fvm_object_create(other,&foreign)==FVM_OK);
  fvm_value_number(&out,73);
  assert(fvm_exec_clip_detached(0,root,&out)==FVM_INVALID_ARGUMENT && fvm_exec_clip_detached(c,plain,&out)==FVM_INVALID_ARGUMENT && fvm_exec_clip_detached(c,foreign,&out)==FVM_INVALID_ARGUMENT && out.number==73);
  assert(fvm_value_string(args,"old")==FVM_OK);fvm_value_number(args+1,2);
  assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK);old=out.object;assert(fvm_gc_pin(c,old)==FVM_OK);
  assert(fvm_exec_clip_detached(c,old,&out)==FVM_OK && !out.boolean);
  fvm_value_boolean(&v,1);assert(fvm_object_set(c,old,"detached",&v)==FVM_OK && fvm_exec_clip_detached(c,old,&out)==FVM_OK && !out.boolean);
  assert(fvm_value_string(args,"fresh")==FVM_OK && fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK);fresh=out.object;
  assert(fvm_exec_clip_detached(c,old,&out)==FVM_OK && out.boolean && fvm_exec_clip_detached(c,fresh,&out)==FVM_OK && !out.boolean);
  assert(fvm_exec_detach_clip(c,old)==FVM_OK && fvm_exec_clip_detached(c,fresh,&out)==FVM_OK && !out.boolean);
  assert(fvm_exec_detach_clip(c,fresh)==FVM_OK && fvm_exec_clip_detached(c,fresh,&out)==FVM_OK && out.boolean);
  assert(fvm_exec_clip_detached(c,root,&out)==FVM_OK && !out.boolean && fvm_gc_collect(c,0)==FVM_OK);
  fvm_context_destroy(other);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
