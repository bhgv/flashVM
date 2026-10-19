/* Only explicitly eligible clip-action event chains grant updateAfterEvent; flags unwind on failure. */
#include "clip_actions_fixture.hpp"
static Bytes redraw_code()
{Bytes b;integer(b,0);text(b,"updateAfterEvent");b.push_back(0x3d);b.push_back(0x17);b.push_back(0);return b;}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=5;version<=8;++version){FVM_Context *c=clip_context(rt);unsigned long mask=version==5?0x1ff:version==6?0x3ffff:0x7ffff;FVM_Object *root=clip_load(c,clip_movie(version,clip_event_table(version,mask,redraw_code(),65)));assert(fvm_movieclip_advance(c,root,1)==FVM_OK && !c->stage_update_pending);FVM_Object *actor=get(c,root,"actor").object;
  for(unsigned long event=1;event<=mask;event<<=1){int pending=77;assert(fvm_movieclip_dispatch_clip_actions(c,actor,event,event==0x20000?65:0)==FVM_OK && fvm_stage_take_update(c,&pending)==FVM_OK);assert(pending==((event&0xf8)!=0));clip_clean(c);}
  for(unsigned phase=0;phase<3;++phase){if(!phase)c->destroying=true;else if(phase==1)c->gc_collecting=true;else c->gc_releasing=true;assert(fvm_movieclip_dispatch_clip_actions(c,actor,1,0)==FVM_INVALID_ARGUMENT);c->destroying=c->gc_collecting=c->gc_releasing=false;clip_clean(c);}fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
