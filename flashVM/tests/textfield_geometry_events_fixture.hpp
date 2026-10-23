/* Geometry event observations are copied only after publication and preserve exact source/order. */
#ifndef FVM_TEST_TEXTFIELD_GEOMETRY_EVENTS_FIXTURE_HPP
#define FVM_TEST_TEXTFIELD_GEOMETRY_EVENTS_FIXTURE_HPP
#include "textfield_scroll_events_fixture.hpp"
#ifdef FVM_HAS_FONTS
struct GeometryEvents {FVM_Object *field;unsigned count,order[128];FVM_TextFieldScrollInfo last;};
struct GeometryObserver {GeometryEvents *events;unsigned id;};
static FVM_Result geometry_observe(FVM_Context *c,FVM_Object *,unsigned n,const FVM_Value *args,FVM_Value *out,void *raw)
{
    GeometryObserver *data=(GeometryObserver*)raw;GeometryEvents *events=data->events;
    assert(n==1 && args[0].type==FVM_VALUE_OBJECT && args[0].object==events->field && events->count<128);
    events->last=scroll_info(c,events->field);events->order[events->count++]=data->id;
    assert(c->gc_frames && c->gc_native_depth && fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);fvm_value_undefined(out);return FVM_OK;
}
static FVM_Object *geometry_listener(FVM_Context *c,GeometryObserver *data)
{FVM_Object *o=0;assert(fvm_object_create(c,&o)==FVM_OK && fvm_object_define_method_rooted(c,o,"onScroller",geometry_observe,data,data->events->field)==FVM_OK && scroll_listen(c,data->events->field,o));return o;}
#endif
#endif
