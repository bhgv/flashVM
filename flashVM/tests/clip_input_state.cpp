/* Current native state precedes delivery; capture flag/GC guard restore across errors and phases. */
#include "clip_input_fixture.hpp"
static FVM_Object *selected;
static unsigned calls;
static FVM_Result code_result;
static FVM_Result inspect(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{
 double x,y;FVM_KeyboardState state;std::memset(&state,0,sizeof(state));state.struct_size=sizeof(state);assert(fvm_input_get_pointer(c,&x,&y)==FVM_OK && x==23 && y==37 && fvm_input_get_keyboard(c,&state)==FVM_OK && state.down[FVM_KEY_LEFT]);assert(c->clip_input_dispatching && fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT && fvm_movieclip_dispatch_clip_input(c,selected,8)==FVM_INVALID_ARGUMENT);++calls;fvm_context_destroy(c);assert(c->global);
 assert(fvm_call(c,"updateAfterEvent",0,0,out)==FVM_OK);if(code_result==FVM_EXCEPTION){FVM_Value v;fvm_value_number(&v,99);return fvm_exception_raise(c,&v);}return code_result;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned fail=0;fail<3;++fail){FVM_Context *c=clip_context(rt);Bytes code;integer(code,0);text(code,"inspect");code.push_back(0x3d);code.push_back(0x17);code.push_back(0);selected=clip_load(c,clip_movie(8,clip_event_table(8,8,code)));assert(fvm_movieclip_advance(c,selected,1)==FVM_OK && fvm_register_function(c,"inspect",inspect,0)==FVM_OK && fvm_input_set_pointer(c,23,37)==FVM_OK);FVM_KeyboardState state=c->keyboard;state.down[FVM_KEY_LEFT]=1;assert(fvm_input_set_keyboard(c,&state)==FVM_OK);calls=0;code_result=fail==0?FVM_OK:fail==1?FVM_ERROR:FVM_EXCEPTION;assert(fvm_movieclip_dispatch_clip_input(c,selected,8)==code_result && calls==1 && c->stage_update_pending);input_clean(c);if(fail==2)assert(fvm_exception_clear(c)==FVM_OK);
  for(unsigned phase=0;phase<3;++phase){if(!phase)c->destroying=true;else if(phase==1)c->gc_collecting=true;else c->gc_releasing=true;assert(fvm_movieclip_dispatch_clip_input(c,selected,8)==FVM_INVALID_ARGUMENT);c->destroying=c->gc_collecting=c->gc_releasing=false;}input_clean(c);fvm_context_destroy(c);
 }fvm_runtime_destroy(rt);
}
