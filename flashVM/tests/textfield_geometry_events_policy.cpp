/* Wrapping, autoSize and multiline expose new ranges before notification; no observers remain lazy. */
#include "textfield_geometry_events_fixture.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    GeometryEvents events;events.field=scroll_field(c);events.count=0;GeometryObserver observer={&events,1};assert(fvm_object_define_method_rooted(c,events.field,"onScroller",geometry_observe,&observer,events.field)==FVM_OK);
    FVM_Value v;fvm_value_boolean(&v,1);assert(fvm_object_set(c,events.field,"wordWrap",&v)==FVM_OK);assert(events.count==1 && events.last.line_count==6 && events.last.maxscroll==5 && events.last.maxhscroll==0);
    assert(fvm_object_set(c,events.field,"wordWrap",&v)==FVM_OK && events.count==1);
    assert(fvm_value_string(&v,"center")==FVM_OK && fvm_object_set(c,events.field,"autoSize",&v)==FVM_OK);assert(events.count==2 && events.last.maxscroll==1 && events.last.clip_height>90);
    fvm_value_boolean(&v,0);assert(fvm_object_set(c,events.field,"multiline",&v)==FVM_UNSUPPORTED);assert(events.count==2 && scroll_info(c,events.field).line_count==6);
    assert(fvm_object_set(c,events.field,"autoSize",&v)==FVM_OK && events.count==2);
    FVM_Object *lazy=0;assert(fvm_textfield_create(c,"Geometry",100,100,&lazy)==FVM_OK && fvm_gc_pin(c,lazy)==FVM_OK);assert(fvm_value_string(&v,"\t")==FVM_OK && fvm_object_set(c,lazy,"text",&v)==FVM_OK);scroll_number(c,lazy,"_height",96);
    assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
    return 0;
}
