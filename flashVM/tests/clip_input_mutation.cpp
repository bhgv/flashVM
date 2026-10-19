/* New children wait, removed siblings skip, and captured ancestry prevents moved-subtree delivery. */
#include "clip_input_fixture.hpp"
static FVM_Object *root,*first,*victim,*newborn;
static unsigned calls;
static FVM_Result mutate(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{
 ++calls;assert(c->clip_input_dispatching && fvm_movieclip_dispatch_clip_input(c,root,8)==FVM_INVALID_ARGUMENT);assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);
 if(calls==1){assert(fvm_object_call(c,victim,"removeMovieClip",0,0,out)==FVM_OK);newborn=input_duplicate(c,first,"newborn",5);}fvm_value_undefined(out);return FVM_OK;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);FVM_Context *c=clip_context(rt);
 Bytes code;integer(code,0);text(code,"mutate");code.push_back(0x3d);code.push_back(0x17);code.push_back(0);root=clip_load(c,clip_movie(8,clip_event_table(8,8,code)));assert(fvm_movieclip_advance(c,root,1)==FVM_OK);first=get(c,root,"actor").object;victim=input_duplicate(c,first,"victim",3);assert(fvm_register_function(c,"mutate",mutate,0)==FVM_OK);calls=0;
 assert(fvm_movieclip_dispatch_clip_input(c,root,8)==FVM_OK && calls==1 && newborn);assert(fvm_movieclip_dispatch_clip_input(c,root,8)==FVM_OK && calls==3);input_clean(c);assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
