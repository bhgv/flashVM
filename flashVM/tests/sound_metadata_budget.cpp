/* Exact root/record/envelope work, unchanged outputs and shared ordinal/dictionary scans. */
#include "sound_metadata_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=1;version<=8;++version)
    {
        rt->config.instruction_limit=0;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);Bytes swf=metadata_movie(version);
        assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
        for(unsigned route=0;route<6;++route)
        {
            unsigned long required=metadata_units(route);
            for(unsigned long limit=0;limit<=required+1;++limit)
            {
                rt->config.instruction_limit=limit;bool failure=limit && limit<required;
                assert(metadata_query(c,route,failure)==(failure?FVM_ERROR:FVM_OK));
                assert(c->avm1_steps==(failure?limit:required) && !c->avm1_execution_depth);
            }
            rt->config.instruction_limit=required;
            {FVM_ExecutionWorkScope work(c);assert(work.step()==FVM_OK);assert(metadata_query(c,route,true)==FVM_ERROR && c->avm1_steps==required && c->avm1_execution_depth==1);}
            rt->config.instruction_limit=0;c->avm1_execution_depth=1;c->avm1_steps=std::numeric_limits<unsigned long>::max();
            assert(metadata_query(c,route,true)==FVM_ERROR && c->avm1_steps==std::numeric_limits<unsigned long>::max());c->avm1_execution_depth=0;
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
