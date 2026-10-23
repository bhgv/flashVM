/* Allocation sweeps distinguish unpublished edits from committed partial observer delivery. */
#define main allocation_reference_main
#include "native_instance_allocation.cpp"
#undef main
#include "textfield_edit_events_fixture.hpp"
int main(int argc,char **)
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned registered=0;registered<2;++registered)for(unsigned mode=0;mode<3;++mode)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);GeometryEvents events;events.field=scroll_field(c);events.count=0;
        if(mode==2){FVM_Value policy;fvm_value_boolean(&policy,1);assert(fvm_object_set(c,events.field,"wordWrap",&policy)==FVM_OK && fvm_object_set(c,events.field,"autoSize",&policy)==FVM_OK);}else{scroll_number(c,events.field,"_width",60);scroll_number(c,events.field,"_height",200);}GeometryObserver own={&events,1},listener={&events,2};assert(fvm_object_define_method_rooted(c,events.field,"onScroller",geometry_observe,&own,events.field)==FVM_OK);if(registered)geometry_listener(c,&listener);const unsigned expected_calls=registered?2U:1U;FVM_Object *format=edit_format(c,20);
        FVM_Value value,args[3],style,out;assert(fvm_value_string(&value,"A")==FVM_OK);fvm_value_number(args,0);fvm_value_number(args+1,8);args[2]=value;fvm_value_undefined(&style);style.type=FVM_VALUE_OBJECT;style.object=format;
        bool success=false;unsigned before=0,after=0;
        for(long boundary=0;boundary<4096 && !success;++boundary)
        {
            assert(edit_text(c,events.field,"AV\nAV\nAV",8)==FVM_OK);events.count=0;allowance=argc>1?-1:boundary;
            FVM_Result result=mode==0?fvm_object_set(c,events.field,"text",&value):mode==1?fvm_object_call(c,events.field,"replaceText",3,args,&out):fvm_object_call(c,events.field,"setTextFormat",1,&style,&out);allowance=-1;
            assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);success=result==FVM_OK;FVM_TextFieldScrollInfo info=scroll_info(c,events.field);unsigned long old_bottom=mode==2?6:3,new_bottom=mode==2?3:1;assert(info.scroll==1 && !info.hscroll && info.maxscroll==1 && !info.maxhscroll);
            if(info.bottom_scroll==old_bottom){assert(!success && !events.count);++before;counted_text(c,events.field,"AV\nAV\nAV",8);}
            else{assert(info.bottom_scroll==new_bottom && events.count<=expected_calls);counted_text(c,events.field,mode==2?"AV\nAV\nAV":"A",mode==2?8:1);if(success)assert(events.count==expected_calls);else{++after;assert(events.count<expected_calls);}}
            assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
        }
        assert(success && (argc>1 || (before && after)));fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
#else
    (void)argc;
#endif
    return 0;
}
