/* bottomScroll changes notify when offsets and maxima agree, after committed content publication. */
#include "textfield_edit_events_fixture.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    GeometryEvents events;events.field=scroll_field(c);events.count=0;scroll_number(c,events.field,"_width",60);scroll_number(c,events.field,"_height",200);FVM_TextFieldScrollInfo before=scroll_info(c,events.field);assert(before.bottom_scroll==3 && before.maxscroll==1 && !before.maxhscroll);
    GeometryObserver own={&events,1},listener={&events,2};assert(fvm_object_define_method_rooted(c,events.field,"onScroller",geometry_observe,&own,events.field)==FVM_OK);geometry_listener(c,&listener);
    assert(edit_text(c,events.field,"A",1)==FVM_OK && events.count==2 && events.last.bottom_scroll==1 && events.order[0]==1 && events.order[1]==2);assert(before.scroll==events.last.scroll && before.hscroll==events.last.hscroll && before.maxscroll==events.last.maxscroll && before.maxhscroll==events.last.maxhscroll);counted_text(c,events.field,"A",1);
    assert(edit_text(c,events.field,"A\nA",3)==FVM_OK && events.count==4 && events.last.bottom_scroll==2 && events.last.maxscroll==1 && !events.last.maxhscroll);
    assert(edit_replace(c,events.field,0,3,"AA\nAA\nAA",8)==FVM_OK && events.count==6 && events.last.bottom_scroll==3 && events.last.maxscroll==1 && !events.last.maxhscroll);
    FVM_Object *format=edit_format(c,20);assert(edit_apply(c,events.field,format)==FVM_OK && events.count==6);
    assert(edit_text(c,events.field,"AA\nAA\nAA",8)==FVM_OK && events.count==6);scroll_info(c,events.field);FVM_Path *path=0;assert(fvm_textfield_path(c,events.field,&path)==FVM_OK);fvm_path_destroy(path);assert(events.count==6);
    assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
    return 0;
}
