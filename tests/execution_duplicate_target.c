/* C89 clone requests use absolute depth, signed wrapping and intrinsic method bypass. */
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
  FVM_Context *c;FVM_Object *root,*clip;FVM_Value a[2],out,source,name,depth,v;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};unsigned mode;
  assert(fvm_context_create(rt,&c)==FVM_OK);if(profile){swf[3]=(unsigned char)(profile+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
  assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
  fvm_value_string(a,"source");fvm_value_number(a+1,1);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,a,&out)==FVM_OK);clip=out.object;
  fvm_value_boolean(&v,0);assert(fvm_object_set(c,clip,"duplicateMovieClip",&v)==FVM_OK);
  for(mode=0;mode<3;++mode)
  {
   fvm_value_string(&source,"source");if(mode==1){source.type=FVM_VALUE_OBJECT;source.object=clip;}
   fvm_value_string(&name,"copy");fvm_value_number(&depth,mode==0?16386.9:mode==1?4294983683.0:0);
   assert(fvm_exec_duplicate_target(c,root,&source,&name,&depth,&name)==FVM_OK && name.type==FVM_VALUE_OBJECT && name.object!=clip);
   assert(fvm_object_call(c,name.object,"getDepth",0,0,&out)==FVM_OK && out.number==(mode==0?2:mode==1?3:-16384));
  }
  fvm_value_string(&source,"source");fvm_value_string(&name,"invalid");fvm_value_number(&depth,-1);assert(fvm_exec_duplicate_target(c,root,&source,&name,&depth,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
  fvm_value_number(&depth,2130706429.0);assert(fvm_exec_duplicate_target(c,root,&source,&name,&depth,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
  fvm_value_number(&depth,2130706428.0);fvm_value_number(&out,73);assert(fvm_exec_duplicate_target(c,root,&source,&name,&depth,&out)==FVM_UNSUPPORTED && out.number==73);
  fvm_value_number(&depth,1064959);assert(fvm_exec_duplicate_target(c,root,&source,&name,&depth,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT);assert(fvm_object_call(c,out.object,"getDepth",0,0,&v)==FVM_OK && v.number==1048575);
  fvm_value_number(&depth,1064960);fvm_value_number(&out,73);assert(fvm_exec_duplicate_target(c,root,&source,&name,&depth,&out)==FVM_UNSUPPORTED && out.number==73);
  fvm_value_number(&depth,2000000);fvm_value_number(&out,73);assert(fvm_exec_duplicate_target(c,root,&source,&name,&depth,&out)==FVM_UNSUPPORTED && out.number==73);
  fvm_value_string(&source,"missing");assert(fvm_exec_duplicate_target(c,root,&source,&name,&depth,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
  fvm_value_string(&source,"");assert(fvm_exec_duplicate_target(c,root,&source,&name,&depth,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
  fvm_value_number(&out,73);assert(fvm_exec_duplicate_target(c,root,0,&name,&depth,&out)==FVM_INVALID_ARGUMENT && out.number==73);
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
