/* Parent-first DFS uses ascending signed child depths rather than hash insertion order. */
#include "movieclip_tick_order_fixture.hpp"
/* Exercise sorted and deliberately shuffled insertion under host and SWF5–8 policies. */
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    for(unsigned version=0;version<=8;++version)
    {
        if(version && version<5)continue;
        for(unsigned reverse=0;reverse<2;++reverse)
        {
            FVM_Context *c=tick_context(runtime,version);FVM_Object *root=tick_root(c);
            FVM_Object *high=0,*low=0,*middle=0;
            if(reverse){high=ordered_child(c,root,"high",10);middle=ordered_child(c,root,"middle",2);low=ordered_child(c,root,"low",-5);}
            else{low=ordered_child(c,root,"low",-5);high=ordered_child(c,root,"high",10);middle=ordered_child(c,root,"middle",2);}
            FVM_Object *grandchild=ordered_child(c,low,"grandchild",0);
            OrderTrace trace={{root,low,grandchild,middle,high,0,0,0},0,5};
            for(unsigned i=0;i<trace.total;++i)
                assert(fvm_object_define_method_rooted(c,trace.expected[i],"onEnterFrame",ordered,&trace,0)==FVM_OK);
            assert(fvm_movieclip_advance(c,root,1)==FVM_OK && trace.count==5);
            trace.count=0;assert(fvm_movieclip_advance(c,root,1)==FVM_OK && trace.count==5);
            tick_clean(c);assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(runtime);
}
