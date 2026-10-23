/* Field-first listener order, source arguments, duplicates, removal and unchanged scroll suppression. */
#include "textfield_scroll_events_fixture.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    ScrollEvents events;events.field=scroll_field(c);events.count=0;events.top=2;events.horizontal=0;
    ScrollObserver own={&events,10},first={&events,20},second={&events,30};
    assert(fvm_object_define_method_rooted(c,events.field,"onScroller",scroll_observe,&own,events.field)==FVM_OK);
    FVM_Object *a=scroll_observer(c,&first),*b=scroll_observer(c,&second);
    assert(!scroll_listen(c,events.field,0));assert(scroll_listen(c,events.field,a) && scroll_listen(c,events.field,b) && scroll_listen(c,events.field,a));
    assert(fvm_gc_collect(c,0)==FVM_OK);scroll_number(c,events.field,"scroll",2);
    assert(events.count==3 && events.trace[0]==10 && events.trace[1]==20 && events.trace[2]==30);
    scroll_number(c,events.field,"scroll",999);scroll_number(c,events.field,"scroll",2);assert(events.count==3);
    events.horizontal=7;scroll_number(c,events.field,"hscroll",7);assert(events.count==6 && events.trace[3]==10 && events.trace[4]==20 && events.trace[5]==30);
    assert(scroll_listen(c,events.field,a,"removeListener") && !scroll_listen(c,events.field,a,"removeListener"));
    assert(fvm_gc_collect(c,0)==FVM_OK);events.top=1;scroll_number(c,events.field,"scroll",1);assert(events.count==8 && events.trace[6]==10 && events.trace[7]==30);
    FVM_TextFieldScrollInfo info=scroll_info(c,events.field);FVM_Path *path=0;assert(fvm_textfield_path(c,events.field,&path)==FVM_OK);fvm_path_destroy(path);assert(info.scroll==1 && events.count==8);
    assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
    return 0;
}
