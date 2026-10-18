/* Loaded detached/replaced clips retain AVM1 parent/root and absolute path provenance. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
#include "../src/standard/movieclip_state.hpp"
static FVM_Object *make_child(FVM_Context *c,FVM_Object *parent,const char *key,double depth)
{FVM_Value a[2],out;fvm_value_string(a,key);fvm_value_number(a+1,depth);assert(fvm_object_call(c,parent,"createEmptyMovieClip",2,a,&out)==FVM_OK);return out.object;}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned profile=0;profile<6;++profile)
 {
  FVM_Context *c=0;FVM_Object *root=0;FVM_Value out,path;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)(profile+3);
  assert(fvm_context_create(rt,&c)==FVM_OK);if(profile)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
  FVM_Object *old=make_child(c,root,"old",1),*nested=make_child(c,old,"nested",0);assert(fvm_gc_pin(c,old)==FVM_OK);
  FVM_Object *replacement=make_child(c,root,"replacement",1);
  assert(old->resource_result.object->properties["parent"].type==FVM_VALUE_NULL);
  assert(fvm_movieclip_preload_display(c,old,true,&out)==FVM_OK && (profile?out.object==root:out.type==FVM_VALUE_NULL));
  assert(fvm_movieclip_preload_display(c,nested,false,&out)==FVM_OK && out.object==(profile?root:old));
  assert(fvm_exec_detach_clip(c,old)==FVM_OK && fvm_exec_detach_clip(c,old)==FVM_OK);
  assert(fvm_movieclip_preload_display(c,old,true,&out)==FVM_OK && (profile?out.object==root:out.type==FVM_VALUE_NULL));
  fvm_value_string(&path,"/replacement");FVM_Object *resolved=0;assert(fvm_movieclip_resolve_path(c,nested,path,&resolved)==FVM_OK && resolved==(profile?replacement:0));
  if(profile)
  {
   /* A removed base has no selected target: unqualified lookup falls back to its retained root. */
   fvm_value_number(&out,73);assert(fvm_object_set(c,root,"marker",&out)==FVM_OK);
   FVM_Object *fn=0;
   /* Explicitly selecting an invalid target exercises the same target-or-root fallback. */
   Bytes body;text(body,"absent");body.push_back(0x20);text(body,"marker");body.push_back(0x1c);end(body);assert(fvm_function_create_avm1(c,body.begin(),body.size(),0,0,0,&fn)==FVM_OK && fvm_function_invoke(c,fn,old,0,0,&out)==FVM_OK && out.number==73);
  }
  assert(fvm_gc_unpin(c,root)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);
  assert(fvm_movieclip_preload_display(c,old,false,&out)==FVM_OK && out.object==(profile?root:old));
  FVM_Context *other=0;assert(fvm_context_create(rt,&other)==FVM_OK);if(profile)assert(fvm_load_memory(other,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(other)==FVM_OK);
  FVM_Channel *channel=0;FVM_Value packet;fvm_value_undefined(&packet);packet.type=FVM_VALUE_OBJECT;packet.object=old;
  assert(fvm_channel_create(&channel)==FVM_OK && fvm_channel_send_value(c,channel,&packet,0)==FVM_OK && fvm_channel_receive_value(other,channel,&packet,0)==FVM_OK && fvm_gc_pin(other,packet.object)==FVM_OK && fvm_gc_collect(other,0)==FVM_OK);
  FVM_Object *copy=packet.object;assert(fvm_movieclip_preload_display(other,copy,false,&out)==FVM_OK);
  if(profile){assert(out.object!=copy && out.object!=root);assert(fvm_movieclip_resolve_path(other,copy,path,&resolved)==FVM_OK && resolved && resolved->owner==other);}else assert(out.object==copy);
  fvm_channel_destroy(channel);fvm_context_destroy(other);
  fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
