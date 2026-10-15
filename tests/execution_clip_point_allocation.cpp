/* Point conversion failures permit only the documented ordered prefix of coordinate writes. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned profile=0;profile<6;++profile)for(unsigned native=0;native<2;++native){
  FVM_Context *c=0;FVM_Object *clip=0,*point=0;FVM_Value v,out;FVM_Affine matrix={2,0,0,3,10,20};assert(fvm_context_create(rt,&c)==FVM_OK);if(profile){unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)(profile+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"clip",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK && fvm_movieclip_set_matrix(c,clip,&matrix)==FVM_OK && fvm_object_create(c,&point)==FVM_OK && fvm_gc_pin(c,point)==FVM_OK);bool success=false;unsigned failures=0;FVM_GCRootFrame *roots=c->gc_frames;
  for(long failure=0;failure<2048 && !success;++failure){fvm_value_number(&v,1);assert(fvm_object_set(c,point,"x",&v)==FVM_OK);fvm_value_number(&v,2);assert(fvm_object_set(c,point,"y",&v)==FVM_OK);fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=point;allowance=failure;FVM_Result r=native?fvm_object_call(c,clip,"localToGlobal",1,&v,&out):fvm_exec_clip_point(c,clip,0,&v);allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);success=r==FVM_OK;if(!success)++failures;FVM_Value x,y;assert(fvm_object_get(c,point,"x",&x)==FVM_OK && fvm_object_get(c,point,"y",&y)==FVM_OK);assert((x.number==1 && y.number==2)||(x.number==12 && (y.number==2 || y.number==26)));if(success)assert(x.number==12 && y.number==26);assert(c->gc_frames==roots && !c->gc_native_depth && !c->script_depth && fvm_gc_collect(c,0)==FVM_OK);}
  assert(success && failures);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
