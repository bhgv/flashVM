/* Captured suffixes survive replacement/append/reentry; failed entries commit their attempt cursor once. */
#include "initializer_progress_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned mutation=1;mutation<=4;++mutation)
    {
        FVM_Context *c=tick_context(rt);FVM_Object *root=init_root(c,8,3);InitProbe p;init_bind(c,root,p);p.mutation=mutation;
        if(mutation==4)fvm_value_number(&p.table->indexed[1],17);
        assert(fvm_movieclip_advance(c,root,1)==(mutation==4?FVM_INVALID_ARGUMENT:FVM_OK));init_clean(c,p);
        assert(p.calls[0]==1 && p.calls[1]==(mutation==4?0U:1U) && p.calls[2]==(mutation==4?0U:1U) && !p.calls[3]);
        assert(fvm_movieclip_advance(c,root,1)==FVM_OK);init_clean(c,p);
        assert(p.calls[0]==1 && p.calls[1]==(mutation==4?0U:1U) && p.calls[2]==1 && p.calls[3]==(mutation==2?1U:0U));
        assert(p.library->swf_initializer_next==(mutation==2?4UL:3UL));
        rt->config.instruction_limit=2;assert(fvm_movieclip_advance(c,root,1)==FVM_OK && c->avm1_steps==2);
        rt->config.instruction_limit=0;fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
