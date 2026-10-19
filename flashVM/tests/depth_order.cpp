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
    /* A full raster-sized reverse list stays below an explicit n log n work bound. */
    repeated.clear();
    for(unsigned long i=65536;i;--i)repeated.push_back(i-1);
    {FVM_ExecutionWorkScope work(c);assert(fvm_depth_order::sort(repeated,work)==FVM_OK);assert(c->avm1_steps<65536UL*40UL);}
    for(size_t i=0;i<repeated.size();++i)assert(repeated[i]==i);
    /* Disabled accounting must preserve an enclosing caller's exhausted budget. */
    c->avm1_steps=71;runtime->config.instruction_limit=1;
    {FVM_ExecutionWorkScope work(c,false);assert(fvm_depth_order::sort(repeated,work)==FVM_OK);assert(c->avm1_steps==71);}
    assert(!c->avm1_execution_depth);fvm_context_destroy(c);fvm_runtime_destroy(runtime);
}
