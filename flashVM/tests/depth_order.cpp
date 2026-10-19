/* Shared heap-sort oracle: odd/even workspaces, duplicates, zero work and exact exhaustion boundary. */
#include "../src/core/depth_order.hpp"
#include <cassert>
/* Check small rotated input sequences without assuming hash ordering or a library sorting algorithm. */
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK && fvm_context_create(runtime,&c)==FVM_OK);
    for(unsigned size=0;size<=31;++size)for(unsigned rotate=0;rotate<(size?size:1);++rotate)
    {
        fvm_detail::vector<unsigned long> values;
        for(unsigned i=0;i<size;++i)values.push_back((size-i+rotate)%size);
        unsigned long units=0;
        {FVM_ExecutionWorkScope work(c);assert(fvm_depth_order::sort(values,work)==FVM_OK);units=c->avm1_steps;}
        for(unsigned i=0;i<size;++i)assert(values[i]==i);
        if(size<2)assert(!units);
        for(unsigned long limit=1;limit<=units;++limit)
        {
            values.clear();for(unsigned i=0;i<size;++i)values.push_back((size-i+rotate)%size);
            runtime->config.instruction_limit=limit;
            {FVM_ExecutionWorkScope work(c);assert(fvm_depth_order::sort(values,work)==(limit==units?FVM_OK:FVM_ERROR));assert(c->avm1_steps==limit);}
        }
        runtime->config.instruction_limit=0;
    }
    fvm_detail::vector<unsigned long> repeated;
    for(unsigned i=0;i<32;++i)repeated.push_back(i%3);
    {FVM_ExecutionWorkScope work(c);assert(fvm_depth_order::sort(repeated,work)==FVM_OK);}
    for(size_t i=1;i<repeated.size();++i)assert(repeated[i-1]<=repeated[i]);
    assert(!c->avm1_execution_depth);fvm_context_destroy(c);fvm_runtime_destroy(runtime);
}
