/* Loaded hitTest points use unlocked root coordinates; host world-space behavior is unchanged. */
#define main movieclip_fixture_main
#include "standard_movieclip.c"
#undef main
#include "flashvm/execution.h"
static void hit_at(FVM_Context *c,FVM_Object *clip,double x,double y,int shape,int expected)
{FVM_Value args[3],out;fvm_value_number(args,x);fvm_value_number(args+1,y);fvm_value_boolean(args+2,shape);assert(fvm_object_call(c,clip,"hitTest",3,args,&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN && out.boolean==expected);}
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned profile;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(profile=0;profile<6;++profile)
 {
  FVM_Context *c;FVM_Object *root,*clip;FVM_Value a[3],out,v;unsigned lock;FVM_Affine transform={2,0,0,2,20,20};
  unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
  assert(fvm_context_create(rt,&c)==FVM_OK);if(profile){swf[3]=(unsigned char)(profile+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
  assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK && fvm_movieclip_set_matrix(c,root,&transform)==FVM_OK);
  fvm_value_string(a,"triangle");fvm_value_number(a+1,0);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,a,&out)==FVM_OK);clip=out.object;fvm_value_number(&v,5);assert(fvm_object_set(c,clip,"_x",&v)==FVM_OK);
  fvm_value_number(a,0xff0000);assert(fvm_object_call(c,clip,"beginFill",1,a,&out)==FVM_OK);coordinate(c,clip,"moveTo",0,0);coordinate(c,clip,"lineTo",10,0);coordinate(c,clip,"lineTo",0,10);assert(fvm_object_call(c,clip,"endFill",0,0,&out)==FVM_OK);
  if(profile)
  {
   const char *paths[]={"triangle","","missing"};unsigned i;
   for(i=0;i<3;++i){fvm_value_string(a,paths[i]);assert(fvm_object_call(c,root,"hitTest",1,a,&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN && !!out.boolean==(i==0));}
   assert(fvm_object_call(c,root,"hitTest",0,0,&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN && !out.boolean);
   fvm_value_undefined(a);a->type=FVM_VALUE_OBJECT;a->object=clip;assert(fvm_object_call(c,root,"hitTest",1,a,&out)==FVM_OK && out.boolean);
   fvm_value_string(a,"triangle");assert(fvm_exec_resolve_clip(c,root,a,0,a)==FVM_OK && a->type==FVM_VALUE_OBJECT && a->object==clip);
   fvm_value_string(a,"");assert(fvm_exec_resolve_clip(c,root,a,0,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);assert(fvm_exec_resolve_clip(c,root,a,1,a)==FVM_OK && a->object==root);
   fvm_value_number(&out,73);assert(fvm_exec_resolve_clip(c,root,0,0,&out)==FVM_INVALID_ARGUMENT && out.number==73);
  }
  for(lock=0;lock<2;++lock)
  {
   fvm_value_boolean(&v,lock);assert(fvm_object_set(c,clip,"_lockroot",&v)==FVM_OK);
   hit_at(c,clip,profile?7:34,profile?2:24,0,1);hit_at(c,clip,profile?7:34,profile?2:24,1,1);
   hit_at(c,clip,profile?13:46,profile?8:36,0,1);hit_at(c,clip,profile?13:46,profile?8:36,1,0);
   hit_at(c,clip,profile?34:7,profile?24:2,0,0);
  }
  fvm_value_number(a,profile?13:46);fvm_value_number(a+1,profile?8:36);fvm_value_string(a+2,"yes");assert(fvm_object_call(c,clip,"hitTest",3,a,&out)==FVM_OK && !!out.boolean==(profile && profile+3<7));
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
