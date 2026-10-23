/* Current formats notify changed ranges; future formats and decoration-only changes stay silent. */
#include "textfield_edit_events_fixture.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    GeometryEvents events;events.field=scroll_field(c);events.count=0;scroll_number(c,events.field,"scroll",2);scroll_number(c,events.field,"hscroll",7);GeometryObserver own={&events,1},listener={&events,2};assert(fvm_object_define_method_rooted(c,events.field,"onScroller",geometry_observe,&own,events.field)==FVM_OK);geometry_listener(c,&listener);
    FVM_Object *format=edit_format(c,20);assert(edit_apply(c,events.field,format)==FVM_OK && events.count==2 && events.last.maxscroll==1 && events.last.maxhscroll==0 && events.last.scroll==1 && events.last.hscroll==0 && counted_metric(c,events.field,"textWidth")==24);
    scroll_number(c,format,"size",60);assert(edit_apply(c,events.field,format)==FVM_OK && events.count==4 && events.last.maxscroll==3 && events.last.maxhscroll==36 && counted_metric(c,events.field,"textWidth")==72);
    scroll_number(c,format,"size",40);assert(edit_apply(c,events.field,format,"setNewTextFormat")==FVM_OK && events.count==4 && counted_metric(c,events.field,"textWidth")==72);
    assert(edit_text(c,events.field,"AV\nAV\nAV",8)==FVM_OK && events.count==6 && events.last.maxscroll==2 && events.last.maxhscroll==12);
    scroll_number(c,format,"color",255);FVM_Value value;fvm_value_boolean(&value,1);assert(fvm_object_set(c,format,"underline",&value)==FVM_OK && edit_apply(c,events.field,format)==FVM_OK && events.count==6);
    assert(fvm_object_set(c,format,"kerning",&value)==FVM_OK && edit_apply(c,events.field,format)==FVM_OK && events.count==8 && events.last.maxhscroll==9);
    scroll_number(c,format,"leading",50);assert(edit_apply(c,events.field,format)==FVM_OK && events.count==10 && events.last.maxscroll==3);
    assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
    return 0;
}
