#define FVM_GEOMETRY_FIXTURE_ONLY
#include "standard_geometry.cpp"
#include "flashvm/serialization.h"
#include "flashvm/capability.h"
#include "flashvm/extensions.h"
#include "../src/standard/color_transform_state.hpp"
static const char *names[]={"redMultiplier","greenMultiplier","blueMultiplier","alphaMultiplier","redOffset","greenOffset","blueOffset","alphaOffset"};
static FVM_Object *color(FVM_Context *c,unsigned n,const double *data) {
    FVM_Value a[8];for(unsigned i=0;i<n;++i)fvm_value_number(a+i,data[i]);FVM_Object *o=0;
    assert(fvm_construct(c,"flash.geom.ColorTransform",n,a,&o)==FVM_OK);assert(fvm_gc_pin(c,o)==FVM_OK);return o;
}
static void values(FVM_Context *c,FVM_Object *o,const double *expected) { for(unsigned i=0;i<8;++i) near(get(c,o,names[i]),expected[i]); }
static void rgb(FVM_Context *c,FVM_Object *o,double n) {FVM_Value v;fvm_value_number(&v,n);assert(fvm_object_set(c,o,"rgb",&v)==FVM_OK);}
static void received(FVM_Context *c,FVM_Object *o) {
    double original[]={2,3,4,0.5,10,20,30,40};values(c,o,original);assert(fvm_gc_collect(c,0)==FVM_OK);values(c,o,original);
    rgb(c,o,0x123456);near(get(c,o,"redMultiplier"),0);near(get(c,o,"blueOffset"),86);near(get(c,o,"alphaMultiplier"),0.5);near(get(c,o,"alphaOffset"),40);near(get(c,o,"rgb"),0x123456);
}
#ifndef FVM_COLOR_TRANSFORM_FIXTURE_ONLY
int main() {
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    double identity[]={1,1,1,1,0,0,0,0};FVM_Object *a=color(c,0,0);values(c,a,identity);
    double defaults[]={9,8,7,6,5,4,3};for(unsigned n=1;n<8;++n) {FVM_Object *partial=color(c,n,defaults);values(c,partial,identity);}
    double original[]={2,3,4,0.5,10,20,30,40}, second[]={3,4,5,0.25,100,200,300,400};
    FVM_Object *first=color(c,8,original),*next=color(c,8,second);FVM_Value arg=ref(first);FVM_Object *copy=0;assert(fvm_construct(c,"flash.geom.ColorTransform",1,&arg,&copy)==FVM_OK);assert(fvm_gc_pin(c,copy)==FVM_OK);values(c,copy,original);
    arg=ref(next);call(c,copy,"concat",1,&arg);double combined[]={6,12,20,0.125,210,620,1230,240};values(c,copy,combined);values(c,first,original);values(c,next,second);
    arg=ref(first);FVM_Object *self=color(c,8,original);arg=ref(self);call(c,self,"concat",1,&arg);double squared[]={4,9,16,0.25,30,80,150,60};values(c,self,squared);
    FVM_Value out=call(c,a,"toString");assert(std::strcmp(out.string,"(redMultiplier=1, greenMultiplier=1, blueMultiplier=1, alphaMultiplier=1, redOffset=0, greenOffset=0, blueOffset=0, alphaOffset=0)")==0);
    FVM_Value string;fvm_value_string(&string,"0.75");assert(fvm_object_set(c,a,"alphaMultiplier",&string)==FVM_OK);near(get(c,a,"alphaMultiplier"),0.75);rgb(c,a,-1);near(get(c,a,"rgb"),0xffffff);near(get(c,a,"alphaMultiplier"),0.75);
    rgb(c,a,4294967296.0+0x112233);near(get(c,a,"rgb"),0x112233);
    FVM_Channel *ch=0;FVM_Value packet=ref(first),message;assert(fvm_channel_create(&ch)==FVM_OK);assert(fvm_channel_send_value(c,ch,&packet,0)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&message,0)==FVM_OK);assert(fvm_gc_pin(other,message.object)==FVM_OK);fvm_channel_destroy(ch);received(other,message.object);
    FVM_Result opened=fvm_channel_open_file("color-transform-test.bin","w",&ch);if(opened==FVM_OK) {assert(fvm_channel_send_value(c,ch,&packet,0)==FVM_OK);fvm_channel_destroy(ch);assert(fvm_channel_open_file("color-transform-test.bin","r",&ch)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&message,0)==FVM_OK);assert(fvm_gc_pin(other,message.object)==FVM_OK);fvm_channel_destroy(ch);received(other,message.object);assert(std::remove("color-transform-test.bin")==0);} else assert(opened==FVM_UNSUPPORTED);
    unsigned char *blob=0;size_t bytes=0;assert(fvm_value_serialize(c,&packet,&blob,&bytes)==FVM_OK);
    size_t corrupt=bytes;for(size_t i=0;i+9<=bytes;++i)if(std::memcmp(blob+i,"redOffset",9)==0){corrupt=i+8;break;}assert(corrupt<bytes);blob[corrupt]='X';
    FVM_Value rejected;assert(fvm_value_deserialize(other,blob,bytes,&rejected)==FVM_INVALID_ARGUMENT);assert(rejected.type==FVM_VALUE_UNDEFINED);blob[corrupt]='t';assert(fvm_value_deserialize(other,blob,bytes,&message)==FVM_OK);assert(fvm_gc_pin(other,message.object)==FVM_OK);received(other,message.object);fvm_serialized_free(blob);
#ifdef OS_PORTS
    FVM_ResourceBroker *broker=0;FVM_Channel *queue=0,*network=0;FVM_Object *wrapped=0;char *uri=0;
    assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);assert(fvm_context_set_broker(c,broker)==FVM_OK);assert(fvm_channel_create(&queue)==FVM_OK);assert(fvm_extensions_wrap_channel(c,queue,&wrapped)==FVM_OK);assert(fvm_broker_export(c,wrapped,&uri)==FVM_OK);
    assert(fvm_channel_open_network(other,uri,&network)==FVM_OK);assert(fvm_channel_send_value(c,queue,&packet,0)==FVM_OK);assert(fvm_channel_receive_value(other,network,&message,0)==FVM_OK);assert(fvm_gc_pin(other,message.object)==FVM_OK);received(other,message.object);
    fvm_channel_release(network);fvm_broker_stop(broker);fvm_channel_release(queue);fvm_broker_release(broker);fvm_serialized_free(uri);
#endif
    double native[8];assert(fvm_color_transform_values(c,first,native));values(c,first,original);
    FVM_Object *data=first->resource_result.object;FVM_Value saved=data->properties["redOffset"];fvm_value_string(&data->properties["redOffset"],"bad");unsigned char *serialized=0;size_t size=0;assert(fvm_value_serialize(c,&packet,&serialized,&size)==FVM_UNSUPPORTED);data->properties["redOffset"]=saved;
    assert(fvm_standard_install(c)==FVM_OK);fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
#endif
