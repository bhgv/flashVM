/* Public dispatch validates owner/type/phase/version/arguments without altering input or timeline. */
#include "movieclip_tick_fixture.hpp"
#include "flashvm/clip_actions.h"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);FVM_Context *c=tick_context(rt,0),*other=tick_context(rt,0);FVM_Object *clip=tick_root(c),*foreign=tick_root(other),*plain=0;assert(fvm_object_create(c,&plain)==FVM_OK);
 assert(fvm_stage_request_update(c)==FVM_OK && fvm_movieclip_dispatch_clip_actions(c,clip,FVM_CLIP_EVENT_MOUSE_MOVE,0)==FVM_OK && c->stage_update_pending);FVM_Value frame;assert(fvm_object_get(c,clip,"_currentframe",&frame)==FVM_OK && frame.number==1);tick_clean(c);
 assert(fvm_movieclip_dispatch_clip_actions(0,clip,1,0)==FVM_INVALID_ARGUMENT && fvm_movieclip_dispatch_clip_actions(c,0,1,0)==FVM_INVALID_ARGUMENT && fvm_movieclip_dispatch_clip_actions(c,foreign,1,0)==FVM_INVALID_ARGUMENT && fvm_movieclip_dispatch_clip_actions(c,plain,1,0)==FVM_INVALID_ARGUMENT);
 c->avm1_steps=77;c->movieclip_sound_busy=true;assert(fvm_movieclip_dispatch_clip_actions(c,clip,1,0)==FVM_INVALID_ARGUMENT && c->avm1_steps==77);c->movieclip_sound_busy=false;
 assert(fvm_movieclip_dispatch_clip_actions(c,clip,0x80000,0)==FVM_INVALID_ARGUMENT && c->avm1_steps==77);const unsigned char swf[]={'F','W','S',4,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_movieclip_dispatch_clip_actions(c,clip,1,0)==FVM_UNSUPPORTED);tick_clean(c);
 fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
