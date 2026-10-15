/* Loaded root-relative bounds/shape hits restore native/root scopes at every allocation failure. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
static void coordinate(FVM_Context *c,FVM_Object *clip,const char *method,double x,double y)
{FVM_Value a[2],out;fvm_value_number(a,x);fvm_value_number(a+1,y);assert(fvm_object_call(c,clip,method,2,a,&out)==FVM_OK);}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(unsigned shape=0;shape<3;++shape)
 {
  FVM_Context *c=0;FVM_Object *root=0;FVM_Value a[3],out,v;FVM_Affine transform={2,0,0,2,20,20};unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK && fvm_movieclip_set_matrix(c,root,&transform)==FVM_OK);
  fvm_value_string(a,"triangle");fvm_value_number(a+1,0);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,a,&out)==FVM_OK);FVM_Object *clip=out.object;fvm_value_number(&v,5);assert(fvm_object_set(c,clip,"_x",&v)==FVM_OK);
  fvm_value_number(a,0xff0000);assert(fvm_object_call(c,clip,"beginFill",1,a,&out)==FVM_OK);coordinate(c,clip,"moveTo",0,0);coordinate(c,clip,"lineTo",10,0);coordinate(c,clip,"lineTo",0,10);assert(fvm_object_call(c,clip,"endFill",0,0,&out)==FVM_OK);
  fvm_value_number(a,7);fvm_value_number(a+1,2);fvm_value_boolean(a+2,shape);bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<2000 && !success;++boundary)
  {
   FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;fvm_value_number(&out,73);FVM_Result r=shape==2?fvm_exec_clip_hit_object(c,clip,clip,&out):fvm_exec_clip_hit_point(c,clip,34,24,shape,&out);allowance=-1;assert(c->gc_frames==roots && !c->gc_native_depth && !c->script_depth);
   if(r==FVM_OK){assert(out.type==FVM_VALUE_BOOLEAN && out.boolean);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==73);++failures;}
   assert(fvm_gc_collect(c,0)==FVM_OK);
  }
  assert(success && failures);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
