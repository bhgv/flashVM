/* Nested legacy frame calls share one limit and preserve completed inner effects on failure. */
#include "frame_blocks_fixture.hpp"
int main()
{
    FVM_Config cfg;assert(fvm_config_init(&cfg)==FVM_OK);
    for(unsigned mode=0;mode<2;++mode)for(unsigned long limit=10;limit<=11;++limit)
    {
        if(mode && !fvm_jit_available())continue;
        FVM_Runtime *rt=0;cfg.instruction_limit=limit;assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
        FVM_Context *c=tick_context(rt);FVM_Object *clip=tick_root(c,2);Bytes outer=frame_call(2),inner=action(2);
        assert(fvm_movieclip_define_frame(c,clip,1,0,outer.begin(),outer.size())==FVM_OK);
        assert(fvm_movieclip_define_frame(c,clip,2,0,inner.begin(),inner.size())==FVM_OK);
        assert(fvm_execution_set_mode(c,mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
        assert(run_frame(c,clip,0)==(limit==10?FVM_ERROR:FVM_OK));
        assert(c->avm1_steps==11);number(c,clip,"seen",2);tick_clean(c);
        if(mode)assert(c->execution_mode.stats.native_entries>0);
        fvm_context_destroy(c);fvm_runtime_destroy(rt);
    }
}
