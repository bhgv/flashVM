/* Hash-copy, identity validation and sorted workspace allocation failures unwind tick scopes. */
#define main tick_depth_allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "movieclip_tick_order_fixture.hpp"
/* Repeat from a fresh graph, checking callback order on every partially completed tick. */
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    bool complete=false;unsigned failures=0;
    for(long boundary=0;boundary<2048 && !complete;++boundary)
    {
        FVM_Context *c=tick_context(runtime);FVM_Object *root=tick_root(c);
        FVM_Object *high=ordered_child(c,root,"high",10),*low=ordered_child(c,root,"low",1),*middle=ordered_child(c,root,"middle",5);
        OrderTrace trace={{root,low,middle,high,0,0,0,0},0,4};
        for(unsigned i=0;i<trace.total;++i)
            assert(fvm_object_define_method_rooted(c,trace.expected[i],"onEnterFrame",ordered,&trace,0)==FVM_OK);
        allowance=boundary;FVM_Result result=fvm_movieclip_advance(c,root,1);allowance=-1;
        assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);tick_clean(c);
        if(result==FVM_OK){complete=true;assert(trace.count==4);}else ++failures;
        trace.count=0;assert(fvm_movieclip_advance(c,root,1)==FVM_OK && trace.count==4);
        assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
    }
    assert(complete && failures);fvm_runtime_destroy(runtime);
}
