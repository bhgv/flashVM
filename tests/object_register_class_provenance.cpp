/* registerClass follows the activation base movie, preserving ambient lookup policy. */
#define main provenance_sprite_fixture_main
#include "swf_sprite_resources.cpp"
#undef main
#define FVM_CLASS_CASE_FIXTURE_ONLY
#include "object_register_class_case.cpp"
#include "../src/standard/movieclip_state.hpp"
#include "../src/standard/movieclip_provenance_constants.h"
static FVM_Object *provenance_root(FVM_Context *c,unsigned version)
{
 Bytes tags,empty;tag(tags,1,empty);tag(tags,0,empty);Bytes swf=movie(tags,1);swf[3]=(unsigned char)version;FVM_Object *root=0;
 assert(fvm_movieclip_load_swf(c,swf.begin(),swf.size(),"root",&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);return root;
}
static FVM_Object *provenance_base,*provenance_selected;
static FVM_Result provenance_name(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{assert(c->avm1_base_clip==provenance_base && c->avm1_target_clip==provenance_selected);return fvm_value_string(out,"Hero");}
static void provenance_register(FVM_Context *c,FVM_Object *base,FVM_Object *selected,FVM_Object *fn)
{
 provenance_base=base;provenance_selected=selected;FVM_Object *key=0;assert(fvm_construct(c,"Object",0,0,&key)==FVM_OK && fvm_object_define_method_rooted(c,key,"toString",provenance_name,0,0)==FVM_OK);
 FVM_Value v=case_object(key);assert(fvm_set_global(c,"registrationName",&v)==FVM_OK);v=case_object(selected);assert(fvm_object_set(c,base,"other",&v)==FVM_OK);
 v=case_object(fn);assert(fvm_set_global(c,"registrationCtor",&v)==FVM_OK);v=case_object(selected);assert(fvm_set_global(c,"selectedClip",&v)==FVM_OK);
 Bytes code;text(code,"other");code.push_back(0x20);variable(code,"registrationCtor");variable(code,"registrationName");integer(code,2);variable(code,"Object");text(code,"registerClass");code.push_back(0x52);code.push_back(0x17);code.push_back(0);
 assert(fvm_avm1_execute_receiver(c,code.begin(),code.size(),base)==FVM_OK);
}
#ifndef FVM_CLASS_PROVENANCE_FIXTURE_ONLY
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned ambient=6;ambient<=8;ambient+=2)for(unsigned source=4;source<=8;++source){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);case_loaded(c,ambient);assert(fvm_standard_install(c)==FVM_OK);
  FVM_Object *base=provenance_root(c,source),*selected=provenance_root(c,source<7?8:6),*fn=case_ctor(c,11),*found=0;unsigned version=0;
  assert(fvm_movieclip_movie_version(c,base,&version)==FVM_OK && version==source);provenance_register(c,base,selected,fn);
  bool same=(ambient<7)==(source<7);assert(fvm_registered_class(c,"Hero",&found)==(same?FVM_OK:FVM_NOT_FOUND));if(same)assert(found==fn);
  case_loaded(c,source);assert(fvm_registered_class(c,"Hero",&found)==FVM_OK && found==fn);
  assert(fvm_registered_class(c,"hERO",&found)==(source<7?FVM_OK:FVM_NOT_FOUND));
  assert(fvm_gc_collect(c,0)==FVM_OK && !c->avm1_target_active && !c->avm1_base_clip && !c->gc_frames && !c->gc_native_depth);
  /* Private metadata ignores public names and validates before narrowing numeric input. */
  FVM_Value value;fvm_value_number(&value,99);assert(fvm_object_set(c,base,"swf_version",&value)==FVM_OK);assert(fvm_movieclip_movie_version(c,base,&version)==FVM_OK && version==source);
  FVM_Object *library=base->resource_result.object->properties["library"].object;FVM_Value saved=library->properties[FVM_MOVIECLIP_LIBRARY_VERSION_NAME];library->properties[FVM_MOVIECLIP_LIBRARY_VERSION_NAME]=value;version=77;
  assert(fvm_movieclip_movie_version(c,base,&version)==FVM_INVALID_ARGUMENT && version==77);library->properties[FVM_MOVIECLIP_LIBRARY_VERSION_NAME]=saved;
  library->properties.erase(FVM_MOVIECLIP_LIBRARY_VERSION_NAME);unsigned ambient_version=0;assert(fvm_movieclip_movie_version(c,base,&ambient_version)==FVM_OK && ambient_version==source);library->properties[FVM_MOVIECLIP_LIBRARY_VERSION_NAME]=saved;
  FVM_Context *other=0;FVM_Channel *ch=0;assert(fvm_context_create(rt,&other)==FVM_OK && fvm_channel_create(&ch)==FVM_OK);case_loaded(other,ambient);assert(fvm_standard_install(other)==FVM_OK);value=case_object(base);FVM_Value copy;
  assert(fvm_channel_send_value(c,ch,&value,0)==FVM_OK && fvm_channel_receive_value(other,ch,&copy,0)==FVM_OK && fvm_gc_pin(other,copy.object)==FVM_OK && fvm_gc_collect(other,0)==FVM_OK);
  assert(fvm_movieclip_movie_version(other,copy.object,&version)==FVM_OK && version==source);FVM_Object *otherfn=case_ctor(other,22);provenance_register(other,copy.object,copy.object,otherfn);
  case_loaded(other,source);assert(fvm_registered_class(other,"Hero",&found)==FVM_OK && found==otherfn);assert(!other->gc_frames && !other->gc_native_depth && !other->avm1_target_active);
  fvm_channel_destroy(ch);fvm_context_destroy(other);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
#endif
