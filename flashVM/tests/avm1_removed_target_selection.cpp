/* Loaded SetTarget/2 cannot reselect detached bases or descendants, but can reach live siblings. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static FVM_Object *expected;
static unsigned observed;
static FVM_Result probe(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{assert(c->avm1_target_clip==expected);++observed;fvm_value_undefined(out);return FVM_OK;}
static FVM_Object *child(FVM_Context *c,FVM_Object *parent,const char *key,double depth)
{FVM_Value a[2],out;fvm_value_string(a,key);fvm_value_number(a+1,depth);assert(fvm_object_call(c,parent,"createEmptyMovieClip",2,a,&out)==FVM_OK);return out.object;}
static void observe(FVM_Context *c,FVM_Object *base,Bytes b)
{text(b,"probe");b.push_back(0x26);b.push_back(0);observed=0;FVM_GCRootFrame *roots=c->gc_frames;assert(fvm_avm1_execute_receiver(c,b.begin(),b.size(),base)==FVM_OK && observed==1 && c->gc_frames==roots && !c->avm1_target_active && !c->avm1_execution_depth);}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned profile=0;profile<6;++profile)
 {
  FVM_Context *c=0;FVM_Object *root=0;FVM_Value value;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)(profile+3);
  assert(fvm_context_create(rt,&c)==FVM_OK);if(profile)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK && fvm_avm1_set_trace_rooted(c,probe,0,0)==FVM_OK);
  FVM_Object *old=child(c,root,"old",1),*nested=child(c,old,"nested",0),*live=child(c,root,"live",2);assert(fvm_gc_pin(c,old)==FVM_OK);
  fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=old;assert(fvm_object_set(c,root,"alias",&value)==FVM_OK && fvm_exec_detach_clip(c,old)==FVM_OK);
  expected=profile?0:old;Bytes b;b.push_back(0x8b);u16(b,1);b.push_back(0);observe(c,old,b);
  b.clear();text(b,"");b.push_back(0x20);observe(c,old,b);
  if(profile){b.clear();b.push_back(0x20);expected=0;observe(c,old,b);}
  expected=profile?0:nested;b.clear();text(b,"alias/nested");b.push_back(0x20);observe(c,root,b);
  if(profile){expected=live;b.clear();text(b,"../live");b.push_back(0x20);observe(c,old,b);}
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
