/* Raw CloneSprite consumes three operands, uses absolute depths and ignores method overrides. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned profile=0;profile<6;++profile)
 {
  FVM_Context *c=0;FVM_Object *root=0;FVM_Value a[2],out,v;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
  assert(fvm_context_create(rt,&c)==FVM_OK);if(profile){swf[3]=(unsigned char)(profile+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
  assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
  fvm_value_string(a,"source");fvm_value_number(a+1,1);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,a,&out)==FVM_OK);FVM_Object *clip=out.object;
  fvm_value_boolean(&v,0);assert(fvm_object_set(c,clip,"duplicateMovieClip",&v)==FVM_OK);fvm_value_number(&v,17);assert(fvm_object_set(c,clip,"_x",&v)==FVM_OK && fvm_object_set(c,clip,"custom",&v)==FVM_OK);
  Bytes b;text(b,"sentinel");integer(b,77);text(b,"source");text(b,"copy");integer(b,16386);b.push_back(0x24);b.push_back(0x1d);b.push_back(0);
  FVM_GCRootFrame *roots=c->gc_frames;assert(fvm_avm1_execute_receiver(c,b.begin(),b.size(),root)==FVM_OK && c->gc_frames==roots && !c->avm1_execution_depth && !c->avm1_target_active);
  assert(fvm_object_get(c,root,"sentinel",&out)==FVM_OK && out.number==77);assert(fvm_object_get(c,root,"copy",&out)==FVM_OK && out.type==FVM_VALUE_OBJECT);FVM_Object *copy=out.object;
  assert(fvm_object_get(c,copy,"_x",&out)==FVM_OK && out.number==17 && fvm_object_get(c,copy,"custom",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);assert(fvm_object_call(c,copy,"getDepth",0,0,&out)==FVM_OK && out.number==2);
  const unsigned char empty[]={0x24,0};assert(fvm_avm1_execute_receiver(c,empty,sizeof(empty),root)==(profile?FVM_OK:FVM_ERROR));
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
