/* Depth preparation consumes the shared work budget and preserves committed parent/earlier child effects. */
#include "movieclip_tick_order_fixture.hpp"
/* Check failure before children, partial ordered delivery and later fresh-call completion. */
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    unsigned failed=0,partial=0,complete=0;
    for(unsigned limit=1;limit<=100;++limit)
    {
        FVM_Context *c=tick_context(runtime);FVM_Object *root=tick_root(c);
        FVM_Object *high=ordered_child(c,root,"high",10),*low=ordered_child(c,root,"low",1),*middle=ordered_child(c,root,"middle",5);
        OrderTrace trace={{root,low,middle,high,0,0,0,0},0,4};
        for(unsigned i=0;i<trace.total;++i)
            assert(fvm_object_define_method_rooted(c,trace.expected[i],"onEnterFrame",ordered,&trace,0)==FVM_OK);
        runtime->config.instruction_limit=limit;
        FVM_Result result=fvm_movieclip_advance(c,root,1);
        if(result==FVM_OK){++complete;assert(trace.count==4);}
        else{assert(result==FVM_ERROR);++failed;if(trace.count>1 && trace.count<4)++partial;}
        tick_clean(c);runtime->config.instruction_limit=0;trace.count=0;
        assert(fvm_movieclip_advance(c,root,1)==FVM_OK && trace.count==4);
        fvm_context_destroy(c);
    }
    assert(failed && partial && complete);fvm_runtime_destroy(runtime);
}
