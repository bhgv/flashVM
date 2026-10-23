/* One work budget covers preparation and callback delivery for each edit operation. */
#include "textfield_edit_events_fixture.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned mode=0;mode<3;++mode)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);GeometryEvents events;events.field=scroll_field(c);events.count=0;
        if(mode==2){FVM_Value policy;fvm_value_boolean(&policy,1);assert(fvm_object_set(c,events.field,"wordWrap",&policy)==FVM_OK && fvm_object_set(c,events.field,"autoSize",&policy)==FVM_OK);}else{scroll_number(c,events.field,"_width",60);scroll_number(c,events.field,"_height",200);}GeometryObserver own={&events,1},listener={&events,2};assert(fvm_object_define_method_rooted(c,events.field,"onScroller",geometry_observe,&own,events.field)==FVM_OK);geometry_listener(c,&listener);FVM_Object *format=edit_format(c,20);
        FVM_Value value,args[3],style,out;assert(fvm_value_string(&value,"A")==FVM_OK);fvm_value_number(args,0);fvm_value_number(args+1,8);args[2]=value;fvm_value_undefined(&style);style.type=FVM_VALUE_OBJECT;style.object=format;
        unsigned before=0,after=0,successes=0;
        for(unsigned limit=1;limit<=64;++limit)
        {
            assert(edit_text(c,events.field,"AV\nAV\nAV",8)==FVM_OK);events.count=0;rt->config.instruction_limit=limit;
            FVM_Result result=mode==0?fvm_object_set(c,events.field,"text",&value):mode==1?fvm_object_call(c,events.field,"replaceText",3,args,&out):fvm_object_call(c,events.field,"setTextFormat",1,&style,&out);rt->config.instruction_limit=0;
            assert(result==FVM_OK || result==FVM_ERROR);bool success=result==FVM_OK;if(success)++successes;FVM_TextFieldScrollInfo info=scroll_info(c,events.field);unsigned long old_bottom=mode==2?6:3,new_bottom=mode==2?3:1;assert(info.scroll==1 && !info.hscroll && info.maxscroll==1 && !info.maxhscroll);
            if(info.bottom_scroll==old_bottom){assert(!success && !events.count);++before;counted_text(c,events.field,"AV\nAV\nAV",8);}
            else{assert(info.bottom_scroll==new_bottom && events.count<=2);counted_text(c,events.field,mode==2?"AV\nAV\nAV":"A",mode==2?8:1);if(success)assert(events.count==2);else{++after;assert(events.count<2);}}
            assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
        }
        assert(successes && before && after);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
