/* Exact event flags, wire widths, key selectors and EnterFrame integration on real SWF5--8 sprites. */
#include "clip_actions_fixture.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=5;version<=8;++version){FVM_Context *c=clip_context(rt);Bytes events;u16(events,0);unsigned long mask=version==5?0x1ff:version==6?0x3ffff:0x7ffff;clip_flags(events,version,mask);unsigned index=0;
  for(unsigned long flag=1;flag<=mask;flag<<=1,++index)clip_record(events,version,flag,action(index+1),65);clip_flags(events,version,0);
  FVM_Object *root=clip_load(c,clip_movie(version,events));assert(fvm_movieclip_advance(c,root,1)==FVM_OK);FVM_Object *actor=get(c,root,"actor").object;number(c,actor,"seen",2);index=0;
  for(unsigned long flag=1;flag<=mask;flag<<=1,++index){assert(fvm_movieclip_dispatch_clip_actions(c,actor,flag,flag==0x20000?65:0)==FVM_OK);number(c,actor,"seen",index+1);clip_clean(c);}
  assert(fvm_movieclip_dispatch_clip_actions(c,actor,0x20000,64)==(version==5?FVM_UNSUPPORTED:FVM_OK));number(c,actor,"seen",version==5?9:version==6?18:19);
  assert(fvm_movieclip_dispatch_clip_actions(c,actor,0,0)==FVM_INVALID_ARGUMENT && fvm_movieclip_dispatch_clip_actions(c,actor,3,0)==FVM_INVALID_ARGUMENT && fvm_movieclip_dispatch_clip_actions(c,actor,2,1)==FVM_INVALID_ARGUMENT && fvm_movieclip_dispatch_clip_actions(c,actor,0x20000,256)==FVM_INVALID_ARGUMENT);
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
