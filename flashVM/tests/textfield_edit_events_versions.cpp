/* Loaded same-byte assignment preserves current formatting; SWF4/5 stay silent and no programmatic onChanged fires. */
#include "textfield_edit_events_fixture.hpp"
#ifdef FVM_HAS_FONTS
static FVM_Result edit_unexpected_change(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{assert(!"programmatic edits must not synthesize onChanged");return FVM_ERROR;}
#endif
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char movie[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};movie[3]=(unsigned char)version;assert(fvm_load_memory(c,movie,sizeof(movie))==FVM_OK);
        GeometryEvents events;events.field=scroll_field(c);events.count=0;GeometryObserver own={&events,1};assert(fvm_object_define_method_rooted(c,events.field,"onScroller",geometry_observe,&own,events.field)==FVM_OK && fvm_object_define_method_rooted(c,events.field,"onChanged",edit_unexpected_change,0,events.field)==FVM_OK);
        FVM_Object *format=edit_format(c,20);assert(edit_apply(c,events.field,format)==FVM_OK && events.count==(version<6?0U:1U) && counted_metric(c,events.field,"textWidth")==24);
        scroll_number(c,format,"size",40);assert(edit_apply(c,events.field,format,"setNewTextFormat")==FVM_OK && edit_text(c,events.field,"AV\nAV\nAV",8)==FVM_OK && events.count==(version<6?0U:1U) && counted_metric(c,events.field,"textWidth")==24);
        assert(edit_text(c,events.field,"AAAA",4)==FVM_OK && events.count==(version<6?0U:2U) && counted_metric(c,events.field,"textWidth")==96);
        assert(edit_replace(c,events.field,0,4,"A",1)==FVM_OK && events.count==(version<6?0U:3U));
        assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);fvm_context_destroy(c);
    }
    FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);GeometryEvents events;events.field=scroll_field(c);events.count=0;
    FVM_Value value;assert(fvm_value_string(&value,"center")==FVM_OK && fvm_object_set(c,events.field,"autoSize",&value)==FVM_OK);double center=counted_metric(c,events.field,"_x")+counted_metric(c,events.field,"_width")/2;
    GeometryObserver own={&events,1};assert(fvm_object_define_method_rooted(c,events.field,"onScroller",geometry_observe,&own,events.field)==FVM_OK && edit_text(c,events.field,"A",1)==FVM_OK && events.count==1);
    assert(counted_metric(c,events.field,"_width")==28 && counted_metric(c,events.field,"_height")==48 && counted_metric(c,events.field,"_x")+counted_metric(c,events.field,"_width")/2==center);
    assert(edit_replace(c,events.field,0,1,"AAAA",4)==FVM_OK && events.count==1 && counted_metric(c,events.field,"_width")==100 && counted_metric(c,events.field,"_x")+counted_metric(c,events.field,"_width")/2==center);
    assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
    return 0;
}
