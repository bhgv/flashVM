#define FVM_PALETTE_FIXTURE_ONLY
#include "bitmap_palette_map.cpp"
#include "flashvm/capability.h"
#include "flashvm/extensions.h"
static inline void set(FVM_Context *c,FVM_Object *o,const char *key,double number)
{
    FVM_Value value;fvm_value_number(&value,number);assert(fvm_object_set(c,o,key,&value)==FVM_OK);
}
static inline FVM_Object *hit_point(FVM_Context *c,double x,double y)
{
    double xy[]={x,y};return region_object(c,"flash.geom.Point",2,xy);
}
static inline void hit_args(FVM_Value *a,FVM_Object *first,double threshold,FVM_Object *other,FVM_Object *second=0,double other_threshold=1)
{
    a[0]=ref(first);fvm_value_number(a+1,threshold);a[2]=ref(other);fvm_value_undefined(a+3);if(second)a[3]=ref(second);fvm_value_number(a+4,other_threshold);
}
static inline bool hits(FVM_Context *c,FVM_Object *image,unsigned n,FVM_Value *a)
{
    FVM_Value v=call(c,image,"hitTest",n,a);assert(v.type==FVM_VALUE_BOOLEAN);return v.boolean!=0;
}
static FVM_Result hit_dispose(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *u)
{
    FVM_Value ignored;assert(fvm_object_call(c,(FVM_Object *)u,"dispose",0,0,&ignored)==FVM_OK);fvm_value_number(out,0);return FVM_OK;
}
static inline void transferred_hit(FVM_Context *c,FVM_Object *packet)
{
    FVM_Value image,first,test;assert(fvm_object_get(c,packet,"image",&image)==FVM_OK);assert(fvm_object_get(c,packet,"first",&first)==FVM_OK);assert(fvm_object_get(c,packet,"test",&test)==FVM_OK);
    FVM_Value a[5];hit_args(a,first.object,128,test.object);assert(hits(c,image.object,3,a));
}
#ifndef FVM_HIT_FIXTURE_ONLY
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    FVM_Object *image=bitmap(c,3,2,true,0),*first=hit_point(c,10,20),*test=hit_point(c,11,20);write(c,image,1,0,0x80000000UL);write(c,image,2,1,0xff000000UL);FVM_Value a[5];hit_args(a,first,128,test);assert(hits(c,image,3,a));fvm_value_number(a+1,129);assert(!hits(c,image,3,a));
    a[2]=ref(hit_point(c,13,20));assert(!hits(c,image,3,a));a[2]=ref(hit_point(c,9,20));assert(!hits(c,image,3,a));a[2]=ref(hit_point(c,10,20));fvm_value_number(a+1,0);assert(!hits(c,image,3,a));
    double box[]={10,20,1,1};FVM_Object *rect=region_object(c,"flash.geom.Rectangle",4,box);a[2]=ref(rect);assert(hits(c,image,3,a));fvm_value_number(a+1,1);assert(!hits(c,image,3,a));set(c,rect,"width",3);assert(hits(c,image,3,a));set(c,rect,"height",-1);assert(!hits(c,image,3,a));set(c,rect,"height",2);set(c,rect,"x",13);assert(!hits(c,image,3,a));
    FVM_Object *comparison=bitmap(c,2,2,true,0),*second=hit_point(c,11,20);write(c,comparison,0,0,0x7f000000UL);hit_args(a,first,128,comparison,second,127);assert(hits(c,image,5,a));fvm_value_number(a+4,128);assert(!hits(c,image,5,a));assert(hits(c,image,4,a));assert(call(c,image,"hitTest",3,a).number==-4);
    hit_args(a,first,1,image,first,1);assert(hits(c,image,5,a));FVM_Object *opaque=bitmap(c,1,1,false,0);hit_args(a,first,128,opaque,second,255);assert(hits(c,image,5,a));
    hit_args(a,first,128,test);a[0]=ref(hit_point(c,4294967306.9,20));assert(hits(c,image,3,a));a[0]=ref(hit_point(c,-2147483648.0,20));a[2]=ref(hit_point(c,2147483647.0,20));assert(!hits(c,image,3,a));
    FVM_Object *plain=0;assert(fvm_object_create(c,&plain)==FVM_OK);assert(fvm_gc_pin(c,plain)==FVM_OK);hit_args(a,plain,1,test);assert(call(c,image,"hitTest",3,a).number==-2);hit_args(a,first,1,plain);assert(call(c,image,"hitTest",3,a).number==-3);set(c,plain,"x",11);set(c,plain,"y",20);assert(hits(c,image,3,a));set(c,plain,"width",1);assert(call(c,image,"hitTest",3,a).number==-3);
    FVM_Object *packet=0;assert(fvm_object_create(c,&packet)==FVM_OK);assert(fvm_gc_pin(c,packet)==FVM_OK);FVM_Value value=ref(image);assert(fvm_object_set(c,packet,"image",&value)==FVM_OK);value=ref(first);assert(fvm_object_set(c,packet,"first",&value)==FVM_OK);value=ref(test);assert(fvm_object_set(c,packet,"test",&value)==FVM_OK);
    FVM_Channel *ch=0;FVM_Value copy;value=ref(packet);assert(fvm_channel_create(&ch)==FVM_OK);assert(fvm_channel_send_value(c,ch,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&copy,0)==FVM_OK);assert(fvm_gc_pin(other,copy.object)==FVM_OK);fvm_channel_destroy(ch);assert(fvm_gc_collect(other,0)==FVM_OK);transferred_hit(other,copy.object);
    FVM_Result opened=fvm_channel_open_file("hit-test.bin","w",&ch);if(opened==FVM_OK){assert(fvm_channel_send_value(c,ch,&value,0)==FVM_OK);fvm_channel_destroy(ch);assert(fvm_channel_open_file("hit-test.bin","r",&ch)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&copy,0)==FVM_OK);assert(fvm_gc_pin(other,copy.object)==FVM_OK);fvm_channel_destroy(ch);assert(fvm_gc_collect(other,0)==FVM_OK);transferred_hit(other,copy.object);assert(std::remove("hit-test.bin")==0);}else assert(opened==FVM_UNSUPPORTED);
#ifdef OS_PORTS
    FVM_ResourceBroker *broker=0;FVM_Channel *queue=0,*network=0;FVM_Object *wrapped=0;char *uri=0;
    assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);assert(fvm_context_set_broker(c,broker)==FVM_OK);assert(fvm_channel_create(&queue)==FVM_OK);
    assert(fvm_extensions_wrap_channel(c,queue,&wrapped)==FVM_OK);assert(fvm_broker_export(c,wrapped,&uri)==FVM_OK);assert(fvm_channel_open_network(other,uri,&network)==FVM_OK);
    assert(fvm_channel_send_value(c,queue,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,network,&copy,0)==FVM_OK);assert(fvm_gc_pin(other,copy.object)==FVM_OK);assert(fvm_gc_collect(other,0)==FVM_OK);transferred_hit(other,copy.object);
    fvm_channel_release(network);fvm_broker_stop(broker);fvm_channel_release(queue);fvm_broker_release(broker);fvm_serialized_free(uri);
#endif
    FVM_Object *disposed=bitmap(c,1,1,true,0xff000000UL),*callback=0;assert(fvm_object_create(c,&callback)==FVM_OK);assert(fvm_gc_pin(c,callback)==FVM_OK);set(c,callback,"y",0);assert(fvm_object_define_accessor_rooted(c,callback,"x",hit_dispose,0,disposed,disposed)==FVM_OK);hit_args(a,callback,1,test);assert(call(c,disposed,"hitTest",3,a).number==-1);
    call(c,comparison,"dispose");hit_args(a,first,1,comparison,second);assert(call(c,image,"hitTest",5,a).number==-3);assert(call(c,disposed,"hitTest",0,0).number==-1);
    fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
#endif
