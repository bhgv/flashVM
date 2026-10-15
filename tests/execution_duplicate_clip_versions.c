/* C89 explicit duplication preserves aliases/errors and bypasses overridden methods. */
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <assert.h>
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned profile;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(profile=0;profile<6;++profile)
 {
  FVM_Context *c,*other;FVM_Object *root,*clip,*initial,*foreign;FVM_Value a[3],out,v;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);if(profile){swf[3]=(unsigned char)(profile+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
  assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK && fvm_object_create(c,&initial)==FVM_OK && fvm_gc_pin(c,initial)==FVM_OK && fvm_object_create(other,&foreign)==FVM_OK);
  fvm_value_string(a,"source");fvm_value_number(a+1,1);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,a,&out)==FVM_OK);clip=out.object;
  fvm_value_boolean(&v,0);assert(fvm_object_set(c,clip,"duplicateMovieClip",&v)==FVM_OK);fvm_value_number(&v,73);assert(fvm_object_set(c,initial,"seed",&v)==FVM_OK);
  fvm_value_string(a,"copy");fvm_value_number(a+1,2);fvm_value_undefined(a+2);a[2].type=FVM_VALUE_OBJECT;a[2].object=initial;
  assert(fvm_exec_duplicate_clip(c,clip,a,a+1,a+2,a)==FVM_OK && a->type==FVM_VALUE_OBJECT && a->object!=clip);assert(fvm_object_get(c,a->object,"seed",&out)==FVM_OK && out.number==73);
  fvm_value_string(a,"rootcopy");assert(fvm_exec_duplicate_clip(c,root,a,a+1,0,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
  fvm_value_number(&out,73);assert(fvm_exec_duplicate_clip(c,foreign,a,a+1,0,&out)==FVM_INVALID_ARGUMENT && out.number==73);assert(fvm_exec_duplicate_clip(c,clip,0,a+1,0,&out)==FVM_INVALID_ARGUMENT && out.number==73);
  fvm_value_number(a+1,-20000);assert(fvm_exec_duplicate_clip(c,clip,a,a+1,0,&out)==FVM_INVALID_ARGUMENT && out.number==73);
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(other);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
