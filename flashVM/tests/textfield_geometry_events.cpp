/* Width/height notify range-only changes, publish clamped offsets and suppress unchanged ranges. */
#include "textfield_geometry_events_fixture.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    GeometryEvents events;events.field=scroll_field(c);events.count=0;scroll_number(c,events.field,"scroll",2);scroll_number(c,events.field,"hscroll",7);
    GeometryObserver own={&events,1},listener={&events,2};assert(fvm_object_define_method_rooted(c,events.field,"onScroller",geometry_observe,&own,events.field)==FVM_OK);geometry_listener(c,&listener);
    scroll_number(c,events.field,"_width",60);assert(events.count==2 && events.last.scroll==2 && !events.last.hscroll && !events.last.maxhscroll && events.order[0]==1 && events.order[1]==2);
    scroll_number(c,events.field,"_height",200);assert(events.count==4 && events.last.scroll==1 && events.last.maxscroll==1);
    scroll_number(c,events.field,"_width",40);assert(events.count==6 && events.last.scroll==1 && events.last.maxhscroll==12);
    scroll_number(c,events.field,"_width",40);scroll_number(c,events.field,"_height",201);scroll_number(c,events.field,"_x",9);scroll_number(c,events.field,"textColor",255);assert(events.count==6);
    scroll_number(c,events.field,"_height",48);assert(events.count==8 && events.last.maxscroll==3 && events.last.scroll==1);
    FVM_Path *path=0;assert(fvm_textfield_path(c,events.field,&path)==FVM_OK);fvm_path_destroy(path);assert(events.count==8 && fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
    return 0;
}
