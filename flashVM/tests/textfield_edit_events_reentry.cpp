/* Outer text/replacement/style publication never overwrites nested text or resurrects removed listeners. */
#include "textfield_edit_events_fixture.hpp"
#ifdef FVM_HAS_FONTS
struct EditReentry {GeometryEvents *events;FVM_Object *listener;bool nested;};
static FVM_Result edit_mutate(FVM_Context *c,FVM_Object *field,unsigned n,const FVM_Value *args,FVM_Value *out,void *raw)
{
    EditReentry *job=(EditReentry*)raw;assert(field==job->events->field && n==1 && args[0].object==field);
    job->events->order[job->events->count++]=job->nested?2:1;
    if(!job->nested){assert(counted_metric(c,field,"textWidth")==24);job->nested=true;assert(scroll_listen(c,field,job->listener,"removeListener") && edit_text(c,field,"AAAA",4)==FVM_OK);}
    fvm_value_undefined(out);return FVM_OK;
}
#endif
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned mode=0;mode<3;++mode)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);GeometryEvents events;events.field=scroll_field(c);events.count=0;GeometryObserver observer={&events,3};FVM_Object *listener=geometry_listener(c,&observer),*format=edit_format(c,20);
        EditReentry job={&events,listener,false};assert(fvm_object_define_method_rooted(c,events.field,"onScroller",edit_mutate,&job,events.field)==FVM_OK);
        assert((mode==0?edit_text(c,events.field,"A",1):mode==1?edit_replace(c,events.field,0,8,"A",1):edit_apply(c,events.field,format))==FVM_OK);
        const unsigned expected[]={1,2,3};assert(events.count==3 && !std::memcmp(events.order,expected,sizeof(expected)) && events.last.maxhscroll==60);counted_text(c,events.field,"AAAA",4);assert(counted_metric(c,events.field,"textWidth")==96);
        assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
