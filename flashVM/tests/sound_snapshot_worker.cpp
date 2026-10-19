/* Real workers stop before source comparison or between a canonical batch's host notifications. */
#include "sound_snapshot_fixture.hpp"
#include "flashvm/worker.h"
struct SoundStop { unsigned calls; };
static FVM_Result stop_sound(FVM_Context *c,FVM_Object *,size_t ordinal,void *raw)
{
    SoundStop *p=static_cast<SoundStop *>(raw);assert(ordinal==p->calls);++p->calls;
    if(p->calls==1){FVM_Worker *self=0;assert(fvm_worker_current(c,&self)==FVM_OK && fvm_worker_stop(self)==FVM_OK);}
    return FVM_OK;
}
static FVM_Result sound_worker(FVM_Context *c,const FVM_Value *input,FVM_Value *out,void *)
{
    assert(fvm_standard_install(c)==FVM_OK);Bytes tags,pcm,empty;u16(pcm,7);pcm.push_back(12);u32(pcm,2);pcm.push_back(0);pcm.push_back(255);tag(tags,14,pcm);
    for(unsigned i=0;i<200;++i)sound_event(tags,0);tag(tags,1,empty);tag(tags,0,empty);Bytes swf=movie(tags,1);
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);FVM_Object *root=0;
    assert(fvm_movieclip_load_swf(c,swf.begin(),swf.size(),"root",&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
    SoundStop p={0};assert(fvm_movieclip_set_sound_callback(c,stop_sound,&p)==FVM_OK);
    if(!input->boolean){FVM_Worker *self=0;assert(fvm_worker_current(c,&self)==FVM_OK && fvm_worker_stop(self)==FVM_OK);}
    assert((input->boolean?fvm_movieclip_advance(c,root,1):sound_snapshot_run(c,root))==FVM_ERROR);tick_clean(c);assert(!c->movieclip_sound_busy);
    if(input->boolean)assert(p.calls>0 && p.calls<200);else assert(!p.calls);
    fvm_value_boolean(out,1);return FVM_OK;
}
int main()
{
    FVM_Config cfg;assert(fvm_config_init(&cfg)==FVM_OK);cfg.instruction_limit=0;
    for(unsigned i=0;i<2;++i){FVM_Value input,out;fvm_value_boolean(&input,i!=0);FVM_Worker *w=0;
        assert(fvm_worker_run(&cfg,sound_worker,&input,0,&w)==FVM_OK && fvm_worker_join(w,&out)==FVM_OK && out.boolean);fvm_worker_destroy(w);}
}
