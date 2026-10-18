#define FVM_GEOMETRY_FIXTURE_ONLY
#include "standard_geometry.cpp"
#include "flashvm/avm1.h"
#include "flashvm/capability.h"
#include "flashvm/extensions.h"
#include "flashvm/serialization.h"
static FVM_Object *rect(FVM_Context *c,double x,double y,double w,double h) { double v[]={x,y,w,h}; return make(c,"flash.geom.Rectangle",4,v); }
static void bounds(FVM_Context *c,FVM_Object *o,double x,double y,double w,double h) { near(get(c,o,"x"),x);near(get(c,o,"y"),y);near(get(c,o,"width"),w);near(get(c,o,"height"),h); }
static void set(FVM_Context *c,FVM_Object *o,const char *name,double n) { FVM_Value v;fvm_value_number(&v,n);assert(fvm_object_set(c,o,name,&v)==FVM_OK); }
static bool boolean(const FVM_Value &v) { assert(v.type==FVM_VALUE_BOOLEAN); return v.boolean!=0; }
static void point(FVM_Context *c,FVM_Object *o,const char *name,double x,double y) { FVM_Value v;assert(fvm_object_get(c,o,name,&v)==FVM_OK);near(get(c,v.object,"x"),x);near(get(c,v.object,"y"),y); }
static void transported(FVM_Context *c,FVM_Object *o) {
    bounds(c,o,1,2,10,20); point(c,o,"topLeft",1,2);point(c,o,"bottomRight",11,22);point(c,o,"size",10,20);
    FVM_Value copy=call(c,o,"clone"); assert(copy.object!=o);assert(fvm_gc_pin(c,copy.object)==FVM_OK);bounds(c,copy.object,1,2,10,20);
    FVM_Value arg=ref(copy.object); assert(boolean(call(c,o,"equals",1,&arg)));
    call(c,copy.object,"setEmpty"); assert(boolean(call(c,copy.object,"isEmpty"))); bounds(c,copy.object,0,0,0,0);
    assert(fvm_gc_collect(c,0)==FVM_OK); point(c,o,"size",10,20);
}
#ifndef FVM_RECTANGLE_FIXTURE_ONLY
static unsigned override_calls=0;
static FVM_Result intersection_override(FVM_Context *,FVM_Object *,unsigned n,const FVM_Value *a,FVM_Value *out,void *) { assert(n==1);++override_calls;*out=a[0];return FVM_OK; }
int main() {
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    const unsigned char actions[]={0x96,9,0,0,'a','v','m','R','e','c','t',0,
      0x96,25,0,7,20,0,0,0,7,10,0,0,0,7,2,0,0,0,7,1,0,0,0,7,4,0,0,0,
      0x96,7,0,0,'f','l','a','s','h',0,0x1c,0x96,6,0,0,'g','e','o','m',0,0x4e,
      0x96,11,0,0,'R','e','c','t','a','n','g','l','e',0,0x53,0x1d,0};
    assert(fvm_avm1_execute(c,actions,sizeof(actions))==FVM_OK);FVM_Value constructed;assert(fvm_get_global(c,"avmRect",&constructed)==FVM_OK);bounds(c,constructed.object,1,2,10,20);
    FVM_Object *r=rect(c,1,2,10,20); transported(c,r);
    FVM_Value xy[2];fvm_value_number(xy,1);fvm_value_number(xy+1,2);assert(boolean(call(c,r,"contains",2,xy)));
    fvm_value_number(xy,11);assert(!boolean(call(c,r,"contains",2,xy)));fvm_value_number(xy,10);fvm_value_number(xy+1,22);assert(!boolean(call(c,r,"contains",2,xy)));
    double pv[]={4,5};FVM_Object *p=make(c,"flash.geom.Point",2,pv);FVM_Value v=ref(p);assert(boolean(call(c,r,"containsPoint",1,&v)));
    FVM_Object *inside=rect(c,2,3,5,6);v=ref(inside);assert(boolean(call(c,r,"containsRectangle",1,&v)));
    FVM_Object *partial=rect(c,5,10,10,20);v=ref(partial);FVM_Value result=call(c,r,"intersection",1,&v);bounds(c,result.object,5,10,6,12);assert(boolean(call(c,r,"intersects",1,&v)));result=call(c,r,"union",1,&v);bounds(c,result.object,1,2,14,28);
    FVM_Object *touch=rect(c,11,2,10,20);v=ref(touch);result=call(c,r,"intersection",1,&v);bounds(c,result.object,0,0,0,0);assert(!boolean(call(c,r,"intersects",1,&v)));
    FVM_Object *empty=make(c,"flash.geom.Rectangle",0,0);bounds(c,empty,0,0,0,0);v=ref(r);result=call(c,empty,"union",1,&v);bounds(c,result.object,1,2,10,20);assert(result.object!=r);
    double single[]={1};FVM_Object *one=make(c,"flash.geom.Rectangle",1,single);FVM_Value undef;assert(fvm_object_get(c,one,"width",&undef)==FVM_OK && undef.type==FVM_VALUE_UNDEFINED);assert(boolean(call(c,one,"isEmpty")));
    FVM_Object *negative=rect(c,1,2,-1,20);assert(boolean(call(c,negative,"isEmpty")));
    FVM_Object *sides=rect(c,1,2,10,20);set(c,sides,"left",3);bounds(c,sides,3,2,8,20);set(c,sides,"top",4);bounds(c,sides,3,4,8,18);set(c,sides,"right",20);set(c,sides,"bottom",30);bounds(c,sides,3,4,17,26);
    v=ref(p);assert(fvm_object_set(c,sides,"topLeft",&v)==FVM_OK);bounds(c,sides,4,5,16,25);assert(fvm_object_set(c,sides,"bottomRight",&v)==FVM_OK);bounds(c,sides,4,5,0,0);assert(fvm_object_set(c,sides,"size",&v)==FVM_OK);bounds(c,sides,4,5,4,5);
    result=call(c,sides,"toString");assert(std::strcmp(result.string,"(x=4, y=5, w=4, h=5)")==0);
    fvm_value_number(xy,2);fvm_value_number(xy+1,3);call(c,sides,"inflate",2,xy);bounds(c,sides,2,2,8,11);v=ref(p);call(c,sides,"inflatePoint",1,&v);bounds(c,sides,-2,-3,16,21);
    call(c,sides,"offset",2,xy);bounds(c,sides,0,0,16,21);call(c,sides,"offsetPoint",1,&v);bounds(c,sides,4,5,16,21);
    assert(fvm_object_define_method_rooted(c,sides,"intersection",intersection_override,0,0)==FVM_OK);v=ref(empty);assert(!boolean(call(c,sides,"intersects",1,&v)));assert(override_calls==1);
    FVM_Channel *ch=0;FVM_Value packet=ref(r),received;assert(fvm_channel_create(&ch)==FVM_OK);assert(fvm_channel_send_value(c,ch,&packet,0)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&received,0)==FVM_OK);assert(fvm_gc_pin(other,received.object)==FVM_OK);fvm_channel_destroy(ch);transported(other,received.object);
    FVM_Result opened=fvm_channel_open_file("rectangle-test.bin","w",&ch);if(opened==FVM_OK) {assert(fvm_channel_send_value(c,ch,&packet,0)==FVM_OK);fvm_channel_destroy(ch);assert(fvm_channel_open_file("rectangle-test.bin","r",&ch)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&received,0)==FVM_OK);assert(fvm_gc_pin(other,received.object)==FVM_OK);fvm_channel_destroy(ch);transported(other,received.object);assert(std::remove("rectangle-test.bin")==0);} else assert(opened==FVM_UNSUPPORTED);
#ifdef OS_PORTS
    FVM_ResourceBroker *broker=0; FVM_Channel *queue=0,*network=0; FVM_Object *wrapped=0; char *uri=0;
    assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK); assert(fvm_context_set_broker(c,broker)==FVM_OK);
    assert(fvm_channel_create(&queue)==FVM_OK); assert(fvm_extensions_wrap_channel(c,queue,&wrapped)==FVM_OK); assert(fvm_broker_export(c,wrapped,&uri)==FVM_OK);
    assert(fvm_channel_open_network(other,uri,&network)==FVM_OK); assert(fvm_channel_send_value(c,queue,&packet,0)==FVM_OK);
    assert(fvm_channel_receive_value(other,network,&received,0)==FVM_OK);assert(fvm_gc_pin(other,received.object)==FVM_OK);transported(other,received.object);
    assert(fvm_channel_send_value(other,network,&received,0)==FVM_OK);assert(fvm_channel_receive_value(c,queue,&received,0)==FVM_OK);assert(fvm_gc_pin(c,received.object)==FVM_OK);transported(c,received.object);
    fvm_channel_release(network);fvm_broker_stop(broker);fvm_channel_release(queue);fvm_broker_release(broker);fvm_serialized_free(uri);
#endif
    assert(fvm_standard_install(c)==FVM_OK);fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
#endif
