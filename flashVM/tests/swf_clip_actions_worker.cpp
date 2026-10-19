/* Actual OS workers stop before dispatch or cooperatively between retained action records. */
#include "clip_actions_fixture.hpp"
#include "flashvm/worker.h"
static unsigned called;
static FVM_Result stop_clip(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{++called;FVM_Worker *self=0;assert(fvm_worker_current(c,&self)==FVM_OK && fvm_worker_stop(self)==FVM_OK);fvm_value_undefined(out);return FVM_OK;}
static FVM_Result run(FVM_Context *c,const FVM_Value *input,FVM_Value *out,void *)
{
 assert(fvm_standard_install(c)==FVM_OK);Bytes code;integer(code,0);text(code,"stopClip");code.push_back(0x3d);code.push_back(0x17);code.push_back(0);Bytes events;u16(events,0);u32(events,1);for(unsigned i=0;i<100;++i)clip_record(events,8,1,code);u32(events,0);
 FVM_Object *root=clip_load(c,clip_movie(8,events));assert(fvm_movieclip_advance(c,root,1)==FVM_OK);FVM_Object *actor=get(c,root,"actor").object;assert(fvm_register_function(c,"stopClip",stop_clip,0)==FVM_OK);called=0;
 if(!input->boolean){FVM_Worker *self=0;assert(fvm_worker_current(c,&self)==FVM_OK && fvm_worker_stop(self)==FVM_OK);}
 assert(fvm_movieclip_dispatch_clip_actions(c,actor,1,0)==FVM_ERROR);assert(input->boolean?(called>0 && called<100):!called);clip_clean(c);fvm_value_boolean(out,1);return FVM_OK;
}
int main()
{
 FVM_Config cfg;assert(fvm_config_init(&cfg)==FVM_OK);cfg.instruction_limit=0;
 for(unsigned i=0;i<2;++i){FVM_Worker *worker=0;FVM_Value in,out;fvm_value_boolean(&in,i);assert(fvm_worker_run(&cfg,run,&in,0,&worker)==FVM_OK && fvm_worker_join(worker,&out)==FVM_OK && out.boolean);fvm_worker_destroy(worker);}
}
