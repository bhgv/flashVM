/* Malformed/cyclic/deep preparation delivers nothing; moved captured ancestors skip descendants. */
#include "clip_input_fixture.hpp"
#include "../src/standard/movieclip_storage.hpp"
#include "../src/standard/movieclip_scene.hpp"
static FVM_Object *branch,*outside;
static FVM_Result move_branch(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{branch->resource_result.object->properties["parent"]=fvm_movieclip_storage::object(outside);fvm_value_undefined(out);return FVM_OK;}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned bad=0;bad<3;++bad){FVM_Context *c=clip_context(rt);FVM_Object *root=0,*actor=input_actor(c,&root),*children=0;assert(fvm_movieclip_scene_children(root,&children)==FVM_OK);if(!bad)children->indexed[999]=children->indexed.begin()->second;else if(bad==1){FVM_Object *o=0;assert(fvm_object_create(c,&o)==FVM_OK);children->indexed[999]=fvm_movieclip_storage::object(o);}else {FVM_Object *parent=actor;for(unsigned i=0;i<65;++i){FVM_Value a[2],out;fvm_value_string(a,"nested");fvm_value_number(a+1,1);assert(fvm_object_call(c,parent,"createEmptyMovieClip",2,a,&out)==FVM_OK);parent=out.object;}}
  assert(fvm_movieclip_dispatch_clip_input(c,root,8)==FVM_INVALID_ARGUMENT && get(c,actor,"seen").type==FVM_VALUE_UNDEFINED);input_clean(c);fvm_context_destroy(c);
 }
 FVM_Context *c=clip_context(rt);Bytes code;integer(code,0);text(code,"moveBranch");code.push_back(0x3d);code.push_back(0x17);code.push_back(0);FVM_Object *root=clip_load(c,clip_movie(8,clip_event_table(8,8,code)));assert(fvm_movieclip_advance(c,root,1)==FVM_OK);branch=get(c,root,"actor").object;FVM_Value a[2],out;fvm_value_string(a,"leaf");fvm_value_number(a+1,1);assert(fvm_object_call(c,branch,"createEmptyMovieClip",2,a,&out)==FVM_OK);FVM_Object *leaf=out.object,*source=input_actor(c);leaf->resource_result.object->properties["clip_actions"]=source->resource_result.object->properties["clip_actions"];
 assert(fvm_movieclip_create(c,"outside",1,&outside)==FVM_OK && fvm_gc_pin(c,outside)==FVM_OK && fvm_register_function(c,"moveBranch",move_branch,0)==FVM_OK);assert(fvm_movieclip_dispatch_clip_input(c,root,8)==FVM_OK && get(c,leaf,"seen").type==FVM_VALUE_UNDEFINED);input_clean(c);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
