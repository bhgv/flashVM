/* Exact byte/capture/delivery ceilings preserve preparation, committed delivery and outer nesting. */
#include "sound_snapshot_fixture.hpp"
#include "../src/core/execution_work_scope.hpp"
#include <limits>
struct SoundWork { unsigned calls; };
static FVM_Result charge_sound(FVM_Context *c,FVM_Object *,size_t,void *raw)
{
    SoundWork *p=static_cast<SoundWork *>(raw);++p->calls;FVM_ExecutionWorkScope work(c);
    for(unsigned i=0;i<5;++i){FVM_Result result=work.step();if(result!=FVM_OK)return result;}
    return FVM_OK;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    size_t source_bytes=sound_movie().size();
    for(unsigned long limit=0;limit<=source_bytes+5;++limit)
    {
        rt->config.instruction_limit=0;FVM_Context *c=tick_context(rt);SoundSnapshot p;FVM_Object *root=sound_snapshot_root(c,p);
        rt->config.instruction_limit=limit;unsigned long required=(unsigned long)c->swf_bytes.size()+4;
        assert(sound_snapshot_run(c,root)==(limit && limit<required?FVM_ERROR:FVM_OK));
        assert(c->avm1_steps==(limit && limit<required?limit:required));
        assert(p.count==(!limit || limit>=required?2U:limit==required-1?1U:0U));
        tick_clean(c);assert(!c->movieclip_sound_busy);fvm_context_destroy(c);
    }
    rt->config.instruction_limit=0;FVM_Context *c=tick_context(rt);SoundSnapshot p;FVM_Object *root=sound_snapshot_root(c,p);
    SoundWork callbacks={0};assert(fvm_movieclip_set_sound_callback(c,charge_sound,&callbacks)==FVM_OK);
    rt->config.instruction_limit=(unsigned long)c->swf_bytes.size()+8;
    assert(sound_snapshot_run(c,root)==FVM_ERROR && callbacks.calls==1 && c->avm1_steps==rt->config.instruction_limit);tick_clean(c);assert(!c->movieclip_sound_busy);
    rt->config.instruction_limit=0;c->avm1_execution_depth=1;c->avm1_steps=std::numeric_limits<unsigned long>::max();
    assert(sound_snapshot_run(c,root)==FVM_ERROR && c->avm1_steps==std::numeric_limits<unsigned long>::max() && c->avm1_execution_depth==1);
    c->avm1_execution_depth=0;tick_clean(c);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
