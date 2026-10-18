/* C89 world query composes each physical ancestor in binary32, independent of root locks. */
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <math.h>
#include <float.h>
static FVM_Object *child(FVM_Context *c,FVM_Object *parent,const char *name)
{FVM_Value a[2],out;fvm_value_string(a,name);fvm_value_number(a+1,1);assert(fvm_object_call(c,parent,"createEmptyMovieClip",2,a,&out)==FVM_OK);return out.object;}
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned profile,native;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(profile=0;profile<6;++profile)for(native=0;native<2;++native){
  FVM_Context *c;FVM_Object *root,*mid,*leaf,*point;FVM_Value v,out;FVM_TwipMatrix world;FVM_Affine half={0.5,0,0,0.5,0,0},local={1,0,0,1,0.15,-0.15};unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
  assert(fvm_context_create(rt,&c)==FVM_OK);if(profile){swf[3]=(unsigned char)(profile+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);mid=child(c,root,"mid");leaf=child(c,mid,"leaf");assert(fvm_movieclip_set_matrix(c,root,&half)==FVM_OK && fvm_movieclip_set_matrix(c,mid,&half)==FVM_OK && fvm_movieclip_set_matrix(c,leaf,&local)==FVM_OK);fvm_value_boolean(&v,1);assert(fvm_object_set(c,leaf,"_lockroot",&v)==FVM_OK);
  assert(fvm_exec_clip_world_matrix(c,leaf,&world)==FVM_OK && world.a==0.25f && world.d==0.25f && world.tx==1 && world.ty==-1);
  assert(fvm_object_create(c,&point)==FVM_OK && fvm_gc_pin(c,point)==FVM_OK);fvm_value_number(&v,0);assert(fvm_object_set(c,point,"x",&v)==FVM_OK && fvm_object_set(c,point,"y",&v)==FVM_OK);fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=point;assert((native?fvm_object_call(c,leaf,"localToGlobal",1,&v,&out):fvm_exec_clip_point(c,leaf,0,&v))==FVM_OK);assert(fvm_object_get(c,point,"x",&out)==FVM_OK && fabs(out.number-(profile?0.05:0.0375))<1e-9 && fvm_object_get(c,point,"y",&out)==FVM_OK && fabs(out.number+(profile?0.05:0.0375))<1e-9);
  assert((native?fvm_object_call(c,leaf,"globalToLocal",1,&v,&out):fvm_exec_clip_point(c,leaf,1,&v))==FVM_OK && fvm_object_get(c,point,"x",&out)==FVM_OK && fabs(out.number)<1e-9);local.xx=1e300;assert(fvm_movieclip_set_matrix(c,leaf,&local)==FVM_OK && fvm_exec_clip_world_matrix(c,leaf,&world)==FVM_OK && world.a!=world.a && world.c!=world.c);if(profile){fvm_value_number(&out,0.15);assert(fvm_object_set(c,point,"x",&out)==FVM_OK && fvm_object_set(c,point,"y",&out)==FVM_OK);assert((native?fvm_object_call(c,leaf,"globalToLocal",1,&v,&out):fvm_exec_clip_point(c,leaf,1,&v))==FVM_OK && fvm_object_get(c,point,"x",&out)==FVM_OK && out.number==0.15);}
  world.tx=77;assert(fvm_exec_clip_world_matrix(c,point,&world)==FVM_INVALID_ARGUMENT && world.tx==77 && fvm_exec_clip_world_matrix(c,leaf,0)==FVM_INVALID_ARGUMENT && fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
