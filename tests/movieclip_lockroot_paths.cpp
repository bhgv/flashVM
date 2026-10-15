/* Root locks bound root/absolute paths while preserving parent and removed-instance ancestry. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/standard/movieclip_state.hpp"
static FVM_Object *make_child(FVM_Context *c,FVM_Object *parent,const char *key,double depth)
{FVM_Value a[2],out;fvm_value_string(a,key);fvm_value_number(a+1,depth);assert(fvm_object_call(c,parent,"createEmptyMovieClip",2,a,&out)==FVM_OK);return out.object;}
static unsigned coercions;
static FVM_Result forbidden(FVM_Context *,const FVM_ScriptFrame *,FVM_Value *,void *)
{++coercions;return FVM_ERROR;}
static void check(FVM_Context *c,FVM_Object *leaf,FVM_Object *root,FVM_Object *parent,FVM_Object *absolute)
{FVM_Value out,path;assert(fvm_movieclip_preload_display(c,leaf,false,&out)==FVM_OK && out.object==root && fvm_movieclip_preload_display(c,leaf,true,&out)==FVM_OK && out.object==parent);FVM_Object *found=0;fvm_value_string(&path,"/match");assert(fvm_movieclip_resolve_path(c,leaf,path,&found)==FVM_OK && found==absolute);}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned profile=0;profile<6;++profile)
 {
  FVM_Context *c=0;FVM_Object *root=0;FVM_Value out,v;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)(profile+3);
  assert(fvm_context_create(rt,&c)==FVM_OK);if(profile)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
  FVM_Object *branch=make_child(c,root,"branch",1),*leaf=make_child(c,branch,"leaf",0),*outer_match=make_child(c,root,"match",2),*inner_match=make_child(c,branch,"match",1);assert(fvm_gc_pin(c,branch)==FVM_OK);
  check(c,leaf,root,branch,outer_match);fvm_value_boolean(&v,1);assert(fvm_object_set(c,branch,"_lockroot",&v)==FVM_OK);check(c,leaf,branch,branch,inner_match);
  FVM_Object *coercible=0;assert(fvm_object_create(c,&coercible)==FVM_OK);attach(c,coercible,"valueOf",forbidden,0);attach(c,coercible,"toString",forbidden,0);fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=coercible;coercions=0;assert(fvm_exec_clip_lock_root(c,1,branch,&v,&out)==FVM_OK && out.boolean && !coercions);
  FVM_Context *other=0;assert(fvm_context_create(rt,&other)==FVM_OK);if(profile)assert(fvm_load_memory(other,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(other)==FVM_OK);FVM_Channel *channel=0;fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=branch;assert(fvm_channel_create(&channel)==FVM_OK && fvm_channel_send_value(c,channel,&v,0)==FVM_OK && fvm_channel_receive_value(other,channel,&v,0)==FVM_OK && fvm_gc_pin(other,v.object)==FVM_OK);FVM_Object *copy=v.object;assert(fvm_object_get(other,copy,"_lockroot",&out)==FVM_OK && out.boolean);fvm_value_boolean(&out,0);assert(fvm_object_set(other,copy,"_lockroot",&out)==FVM_OK && fvm_object_get(other,copy,"_lockroot",&out)==FVM_OK && !out.boolean);fvm_channel_destroy(channel);fvm_context_destroy(other);
  assert(fvm_exec_detach_clip(c,branch)==FVM_OK);check(c,leaf,branch,branch,inner_match);fvm_value_boolean(&v,0);assert(fvm_exec_clip_lock_root(c,1,branch,&v,&out)==FVM_OK);check(c,leaf,profile?root:branch,branch,profile?outer_match:inner_match);
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
