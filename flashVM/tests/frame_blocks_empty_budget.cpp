/* Empty DoAction blocks still charge capture/entry; no AVM1 instruction is needed to fail copying. */
#include "frame_blocks_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    FVM_Context *c=tick_context(rt);FVM_Object *clip=tick_root(c),*segments=frame_segments(c,clip);FVM_Value empty;
    assert(fvm_value_string(&empty,"")==FVM_OK);
    for(unsigned i=0;i<40;++i)segments->indexed[i]=empty;
    for(unsigned long limit=1;limit<=40;++limit)
    {
        rt->config.instruction_limit=limit;
        assert(run_frame(c,clip,0)==FVM_ERROR);
        assert(c->avm1_steps==limit);tick_clean(c);
    }
    rt->config.instruction_limit=0;assert(run_frame(c,clip,0)==FVM_OK && c->avm1_steps==120);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
