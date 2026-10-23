/* Native work limits share preparation/delivery budgets and preserve precommit versus committed semantics. */
#include "textfield_geometry_events_fixture.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    GeometryEvents events;events.field=scroll_field(c);events.count=0;GeometryObserver own={&events,1},listener={&events,2};assert(fvm_object_define_method_rooted(c,events.field,"onScroller",geometry_observe,&own,events.field)==FVM_OK);geometry_listener(c,&listener);
    unsigned before=0,after=0,success=0;
    for(unsigned limit=1;limit<=64;++limit)
    {
        rt->config.instruction_limit=0;scroll_number(c,events.field,"_width",40);events.count=0;
        FVM_Value value;fvm_value_number(&value,60);rt->config.instruction_limit=limit;FVM_Result result=fvm_object_set(c,events.field,"_width",&value);rt->config.instruction_limit=0;
        assert(result==FVM_OK || result==FVM_ERROR);double width=counted_metric(c,events.field,"_width");
        if(result==FVM_OK){++success;assert(width==60 && events.count==2);}else if(width==40){++before;assert(!events.count);}else{++after;assert(width==60 && events.count<2);}
        assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
    }
    assert(before && after && success && fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
    return 0;
}
