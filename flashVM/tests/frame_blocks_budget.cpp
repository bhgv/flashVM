/* A direct frame call no longer resets the instruction budget between its independent blocks. */
#include "frame_blocks_fixture.hpp"
int main()
{
    FVM_Config cfg;assert(fvm_config_init(&cfg)==FVM_OK);
    for(unsigned route=0;route<=4;++route)
    for(unsigned long limit=1;limit<=25;++limit)
    {
        if(route==3 && !fvm_jit_available())continue;
        FVM_Runtime *rt=0;cfg.instruction_limit=limit;assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
        FVM_Context *c=tick_context(rt);FVM_Object *clip=tick_root(c),*segments=frame_segments(c,clip);
        segments->indexed[0]=frame_hex(c,action(1));segments->indexed[1]=frame_hex(c,action(2));
        unsigned long overhead=route==2 || route==3?2UL:route==4?1UL:0UL;
        /* action(n) has six executed instructions, including End. Two captures + two entries. */
        unsigned long required=16+overhead+(route>=2?1UL:0UL);
        FVM_Result result=run_frame(c,clip,route);
        assert(result==(limit<required?FVM_ERROR:FVM_OK));
        assert(result==FVM_OK?c->avm1_steps==required:(c->avm1_steps>=limit && c->avm1_steps<=limit+1));tick_clean(c);
        FVM_Value seen;assert(fvm_object_get(c,clip,"seen",&seen)==FVM_OK);
        if(limit<overhead+8)assert(seen.type==FVM_VALUE_UNDEFINED);
        if(result==FVM_OK)assert(seen.number==2);
        if(route<2 && limit==10)assert(seen.type==FVM_VALUE_NUMBER && seen.number==1);
        fvm_context_destroy(c);fvm_runtime_destroy(rt);
    }
}
