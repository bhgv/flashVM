/* Broadcast all five global input records without visibility/focus filtering or timeline movement. */
#include "clip_input_fixture.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=5;version<=8;++version){FVM_Context *c=clip_context(rt);Bytes table=clip_event_table(version,0xf8,action(42));FVM_Object *root=clip_load(c,clip_movie(version,table));assert(fvm_movieclip_advance(c,root,1)==FVM_OK);FVM_Object *a=get(c,root,"actor").object,*b=input_duplicate(c,a,"b",3);FVM_Value v;fvm_value_number(&v,0);assert(fvm_object_set(c,b,"_visible",&v)==FVM_OK && fvm_object_call(c,b,"stop",0,0,&v)==FVM_OK);
  for(unsigned long event=8;event<=128;event<<=1){assert(fvm_movieclip_dispatch_clip_input(c,root,event)==FVM_OK);number(c,a,"seen",42);number(c,b,"seen",42);number(c,root,"_currentframe",1);input_clean(c);}
  assert(fvm_movieclip_dispatch_clip_input(c,root,0)==FVM_INVALID_ARGUMENT && fvm_movieclip_dispatch_clip_input(c,root,24)==FVM_INVALID_ARGUMENT && fvm_movieclip_dispatch_clip_input(c,root,0x400)==FVM_INVALID_ARGUMENT);
  assert(fvm_movieclip_dispatch_clip_input(0,root,8)==FVM_INVALID_ARGUMENT && fvm_movieclip_dispatch_clip_input(c,0,8)==FVM_INVALID_ARGUMENT);assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
