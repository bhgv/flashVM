#define FVM_DISSOLVE_FIXTURE_ONLY
#include "bitmap_dissolve.cpp"
#include "../src/standard/blur.hpp"
static inline FVM_Object *blur(FVM_Context *c,double x=4,double y=4,double q=1)
{
    FVM_Value a[3];fvm_value_number(a,x);fvm_value_number(a+1,y);fvm_value_number(a+2,q);FVM_Object *o=0;assert(fvm_construct(c,"flash.filters.BlurFilter",3,a,&o)==FVM_OK);assert(fvm_gc_pin(c,o)==FVM_OK);return o;
}
static inline FVM_Value apply_blur(FVM_Context *c,FVM_Object *dest,FVM_Object *source,FVM_Object *filter,FVM_Object *rect=0,FVM_Object *point=0)
{
    FVM_Value a[4];a[0]=ref(source);a[1]=ref(rect?rect:dissolve_rect(c,0,0,1,1));a[2]=ref(point?point:hit_point(c,3,3));a[3]=ref(filter);return call(c,dest,"applyFilter",4,a);
}
static inline void impulse(FVM_Context *c,FVM_Object *image,unsigned quality)
{
    const unsigned weights[]={1,2,3,2,1};
    for(unsigned y=0;y<7;++y)for(unsigned x=0;x<7;++x){unsigned long alpha=0;if(quality==1 && x>=2 && x<=4 && y>=2 && y<=4)alpha=28;if(quality==2 && x>=1 && x<=5 && y>=1 && y<=5)alpha=(unsigned long)std::floor(255.0*weights[x-1]*weights[y-1]/81+0.5);assert(read(c,image,x,y)==(alpha?(alpha<<24)|0xff0000UL:0));}
}
static FVM_Result blur_reentrant(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *u)
{
    FVM_Value value;fvm_value_number(&value,9);assert(fvm_object_set(c,(FVM_Object *)u,"blurY",&value)==FVM_OK);fvm_value_number(out,7);return FVM_OK;
}
static inline void blur_packet(FVM_Context *c,FVM_Object *bundle)
{
    assert(fvm_gc_pin(c,bundle)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);FVM_Value f,s,d;assert(fvm_object_get(c,bundle,"filter",&f)==FVM_OK);assert(fvm_object_get(c,bundle,"source",&s)==FVM_OK);assert(fvm_object_get(c,bundle,"dest",&d)==FVM_OK);FVM_Object *filter=f.object;double values[3];assert(fvm_blur_values(c,filter,values) && values[0]==3 && values[1]==3 && values[2]==2);FVM_Value cloned=call(c,filter,"clone");assert(cloned.object!=filter && fvm_blur_values(c,cloned.object,values));
    FVM_Object *point=0;assert(fvm_object_create(c,&point)==FVM_OK);assert(fvm_gc_pin(c,point)==FVM_OK);set(c,point,"x",3);set(c,point,"y",3);FVM_Value args[]={s,ref(dissolve_rect(c,0,0,1,1)),ref(point),f};call(c,d.object,"applyFilter",4,args);impulse(c,d.object,2);
    FVM_Value v;fvm_value_number(&v,5);assert(fvm_object_set(c,cloned.object,"blurX",&v)==FVM_OK);assert(fvm_blur_values(c,filter,values) && values[0]==3);
}
#ifndef FVM_BLUR_FIXTURE_ONLY
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    FVM_Object *filter=blur(c),*source=bitmap(c,1,1,true,0xffff0000UL),*dest=bitmap(c,7,7,true,0);double options[3];assert(fvm_blur_values(c,filter,options) && options[0]==4 && options[1]==4 && options[2]==1);
    FVM_Value v;fvm_value_number(&v,3);assert(fvm_object_set(c,filter,"blurX",&v)==FVM_OK);assert(fvm_object_set(c,filter,"blurY",&v)==FVM_OK);apply_blur(c,dest,source,filter);impulse(c,dest,1);
    FVM_Object *twice=blur(c,3,3,2),*second=bitmap(c,7,7,true,0);apply_blur(c,second,source,twice);impulse(c,second,2);
    FVM_Object *even=blur(c,4,4,1),*fractional=bitmap(c,7,7,true,0);apply_blur(c,fractional,source,even);for(unsigned y=0;y<7;++y)for(unsigned x=0;x<7;++x){double wx=x>=2 && x<=4?1:x==1 || x==5?0.5:0;double wy=y>=2 && y<=4?1:y==1 || y==5?0.5:0;unsigned long alpha=(unsigned long)std::floor(255*wx*wy/16+0.5);assert(read(c,fractional,x,y)==(alpha?(alpha<<24)|0xff0000UL:0));}
    FVM_Object *mixed_source=bitmap(c,2,1,true,0),*mixed_dest=bitmap(c,7,7,true,0),*horizontal=blur(c,3,1,1);write(c,mixed_source,0,0,0xffff0000UL);write(c,mixed_source,1,0,0x8000ff00UL);apply_blur(c,mixed_dest,mixed_source,horizontal,dissolve_rect(c,0,0,2,1));assert(read(c,mixed_dest,2,3)==0x55ff0000UL);assert(read(c,mixed_dest,3,3)==0x80aa5500UL);assert(read(c,mixed_dest,4,3)==0x80aa5500UL);assert(read(c,mixed_dest,5,3)==0x2b00ff00UL);
    FVM_Object *subpixel=blur(c,2.5,1,1),*subpixel_dest=bitmap(c,7,7,true,0);apply_blur(c,subpixel_dest,source,subpixel);assert(read(c,subpixel_dest,2,3)==0x4dff0000UL);assert(read(c,subpixel_dest,3,3)==0x66ff0000UL);assert(read(c,subpixel_dest,4,3)==0x4dff0000UL);
    FVM_Object *identity=blur(c,255,255,0),*exact=bitmap(c,7,7,true,0);apply_blur(c,exact,source,identity);assert(read(c,exact,3,3)==0xffff0000UL && read(c,exact,2,3)==0);
    FVM_Object *opaque=bitmap(c,7,7,false,0);apply_blur(c,opaque,source,filter);assert(read(c,opaque,3,3)==0xffff0000UL);
    FVM_Object *rect=dissolve_rect(c,10,20,30,40);FVM_Value bounds_args[]={ref(rect),ref(twice)};FVM_Value bound=call(c,dest,"generateFilterRect",2,bounds_args);const char *keys[]={"x","y","width","height"};const double expected[]={8,18,34,44};for(unsigned i=0;i<4;++i){assert(fvm_object_get(c,bound.object,keys[i],&v)==FVM_OK && v.number==expected[i]);}
    FVM_Object *original=bitmap(c,5,5,true,0),*alias=bitmap(c,5,5,true,0),*copy=bitmap(c,5,5,true,0);write(c,original,2,2,0x80ff0000UL);write(c,alias,2,2,0x80ff0000UL);rect=dissolve_rect(c,0,0,5,5);FVM_Object *point=hit_point(c,0,0);apply_blur(c,alias,alias,twice,rect,point);apply_blur(c,copy,original,twice,rect,point);for(unsigned y=0;y<5;++y)for(unsigned x=0;x<5;++x)assert(read(c,alias,x,y)==read(c,copy,x,y));
    FVM_Object *coercer=0;assert(fvm_object_create(c,&coercer)==FVM_OK);assert(fvm_gc_pin(c,coercer)==FVM_OK);assert(fvm_object_define_method_rooted(c,coercer,"valueOf",blur_reentrant,filter,filter)==FVM_OK);v=ref(coercer);assert(fvm_object_set(c,filter,"blurX",&v)==FVM_OK);assert(fvm_blur_values(c,filter,options) && options[0]==7 && options[1]==9);
    fvm_value_number(&v,-20);assert(fvm_object_set(c,filter,"blurX",&v)==FVM_OK);fvm_value_number(&v,999);assert(fvm_object_set(c,filter,"blurY",&v)==FVM_OK);fvm_value_number(&v,2.9);assert(fvm_object_set(c,filter,"quality",&v)==FVM_OK);assert(fvm_blur_values(c,filter,options) && options[0]==0 && options[1]==255 && options[2]==2);
    fvm_value_number(&v,std::numeric_limits<double>::infinity());assert(fvm_object_set(c,filter,"blurX",&v)==FVM_INVALID_ARGUMENT);assert(fvm_blur_values(c,filter,options) && options[0]==0);
    unsigned long before=(unsigned long)read(c,dest,3,3),limit=rt->config.memory_limit_bytes;FVM_Value a[]={ref(source),ref(dissolve_rect(c,0,0,1,1)),ref(hit_point(c,3,3)),ref(twice)},out;rt->config.memory_limit_bytes=1;assert(fvm_object_call(c,dest,"applyFilter",4,a,&out)==FVM_OUT_OF_MEMORY);rt->config.memory_limit_bytes=limit;assert(read(c,dest,3,3)==before);
    FVM_Object *bundle=0;assert(fvm_object_create(c,&bundle)==FVM_OK);assert(fvm_gc_pin(c,bundle)==FVM_OK);FVM_Value edge=ref(twice);assert(fvm_object_set(c,bundle,"filter",&edge)==FVM_OK);edge=ref(source);assert(fvm_object_set(c,bundle,"source",&edge)==FVM_OK);edge=ref(dest);assert(fvm_object_set(c,bundle,"dest",&edge)==FVM_OK);
    FVM_Channel *ch=0;FVM_Value value=ref(bundle),restored;assert(fvm_channel_create(&ch)==FVM_OK);assert(fvm_channel_send_value(c,ch,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&restored,0)==FVM_OK);blur_packet(other,restored.object);fvm_channel_destroy(ch);
    assert(fvm_channel_open_file("blur-test.bin","w",&ch)==FVM_OK);assert(fvm_channel_send_value(c,ch,&value,0)==FVM_OK);fvm_channel_destroy(ch);assert(fvm_channel_open_file("blur-test.bin","r",&ch)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&restored,0)==FVM_OK);fvm_channel_destroy(ch);blur_packet(other,restored.object);assert(std::remove("blur-test.bin")==0);
#ifdef OS_PORTS
    FVM_ResourceBroker *broker=0;FVM_Channel *queue=0,*network=0;FVM_Object *wrapped=0;char *uri=0;assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);assert(fvm_context_set_broker(c,broker)==FVM_OK);assert(fvm_channel_create(&queue)==FVM_OK);assert(fvm_extensions_wrap_channel(c,queue,&wrapped)==FVM_OK);assert(fvm_broker_export(c,wrapped,&uri)==FVM_OK);assert(fvm_channel_open_network(other,uri,&network)==FVM_OK);assert(fvm_channel_send_value(c,queue,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,network,&restored,0)==FVM_OK);blur_packet(other,restored.object);fvm_channel_release(network);fvm_broker_stop(broker);fvm_channel_release(queue);fvm_broker_release(broker);fvm_serialized_free(uri);
#endif
    twice->resource_result.object->properties["quality"].number=16;unsigned char *data=0;size_t size=0;assert(fvm_value_serialize(c,&value,&data,&size)==FVM_UNSUPPORTED);twice->resource_result.object->properties["quality"].number=2;
    fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
#endif
