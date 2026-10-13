#define FVM_NOISE_FIXTURE_ONLY
#include "bitmap_noise.cpp"
static inline FVM_Object *dissolve_rect(FVM_Context *c,double x,double y,double w,double h)
{
    FVM_Object *o=0;assert(fvm_object_create(c,&o)==FVM_OK);assert(fvm_gc_pin(c,o)==FVM_OK);set(c,o,"x",x);set(c,o,"y",y);set(c,o,"width",w);set(c,o,"height",h);return o;
}
static inline void dissolve_args(FVM_Value *a,FVM_Object *source,FVM_Object *rect,FVM_Object *point,double seed=0,double count=3,double color=0xffabcdefUL)
{
    a[0]=ref(source);a[1]=ref(rect);a[2]=ref(point);fvm_value_number(a+3,seed);fvm_value_number(a+4,count);fvm_value_number(a+5,color);
}
static inline void dissolve_packet(FVM_Context *c,FVM_Object *image)
{
    assert(fvm_gc_pin(c,image)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);
    FVM_Object *point=0;assert(fvm_object_create(c,&point)==FVM_OK);assert(fvm_gc_pin(c,point)==FVM_OK);set(c,point,"x",100);set(c,point,"y",100);
    FVM_Value a[6];dissolve_args(a,image,dissolve_rect(c,0,0,4,3),point,0,12,0xffabcdefUL);
    call(c,image,"pixelDissolve",6,a);for(unsigned y=0;y<3;++y)for(unsigned x=0;x<4;++x)assert(read(c,image,x,y)==0xffabcdefUL);
}
#ifndef FVM_DISSOLVE_FIXTURE_ONLY
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    FVM_Object *source=bitmap(c,4,3,true,0xff123456UL),*dest=bitmap(c,4,3,true,0xff000000UL),*rect=dissolve_rect(c,0,0,4,3),*point=hit_point(c,0,0);FVM_Value a[6],out;
    const unsigned order[]={9,6,11,0,2,1,3,8,5,10,7,4};const unsigned cursors[]={1,2,3,4,6,9,11,12,13,14,15,0};
    unsigned long random=c->random_state;double seed=0;
    for(unsigned i=0;i<12;++i){dissolve_args(a,source,rect,point,seed,1);seed=call(c,dest,"pixelDissolve",6,a).number;assert(seed==cursors[i]);for(unsigned k=0;k<12;++k){bool selected=false;for(unsigned j=0;j<=i;++j)selected|=k==order[j];assert(read(c,dest,k%4,k/4)==(selected?0xff123456UL:0xff000000UL));}}assert(c->random_state==random);
    FVM_Object *all=bitmap(c,4,3,true,0xff000000UL);dissolve_args(a,source,rect,point,0,1000);assert(call(c,all,"pixelDissolve",6,a).number==0);for(unsigned y=0;y<3;++y)for(unsigned x=0;x<4;++x)assert(read(c,all,x,y)==read(c,dest,x,y));
    dissolve_args(a,dest,rect,hit_point(c,1000,1000),0,12,0x80010203UL);call(c,dest,"pixelDissolve",6,a);for(unsigned y=0;y<3;++y)for(unsigned x=0;x<4;++x)assert(read(c,dest,x,y)==0x80010203UL);
    FVM_Object *opaque=bitmap(c,4,3,false,0);dissolve_args(a,opaque,rect,point,-1,12,0x00010203UL);call(c,opaque,"pixelDissolve",6,a);assert(read(c,opaque,0,0)==0xff010203UL);
    dissolve_args(a,dest,rect,point,0,12,0x00010203UL);call(c,dest,"pixelDissolve",6,a);assert(read(c,dest,0,0)==0);
    FVM_Object *clipped=bitmap(c,4,3,true,0xff999999UL);dissolve_args(a,source,dissolve_rect(c,-1,0,4,3),point,0,99);call(c,clipped,"pixelDissolve",6,a);for(unsigned y=0;y<3;++y){assert(read(c,clipped,0,y)==0xff999999UL);for(unsigned x=1;x<4;++x)assert(read(c,clipped,x,y)==0xff123456UL);}
    dissolve_args(a,source,rect,point,-1,0);assert(call(c,clipped,"pixelDissolve",6,a).number==15);dissolve_args(a,source,rect,point,4294967295.0,0);assert(call(c,clipped,"pixelDissolve",6,a).number==15);
    dissolve_args(a,source,rect,point,0,-1);assert(fvm_object_call(c,clipped,"pixelDissolve",6,a,&out)==FVM_INVALID_ARGUMENT);
    unsigned long before=(unsigned long)read(c,clipped,0,0),limit=rt->config.memory_limit_bytes;rt->config.memory_limit_bytes=95;dissolve_args(a,source,rect,point,0,3);assert(fvm_object_call(c,clipped,"pixelDissolve",6,a,&out)==FVM_OUT_OF_MEMORY);rt->config.memory_limit_bytes=limit;assert(read(c,clipped,0,0)==before);
    FVM_Object *small=bitmap(c,1,1,true,0xff000000UL);dissolve_args(a,small,dissolve_rect(c,0,0,1,1),point,0,1);call(c,small,"pixelDissolve",6,a);assert(read(c,small,0,0)==0xffabcdefUL);
    FVM_Object *defaulted=bitmap(c,30,2,true,0xff000000UL);dissolve_args(a,defaulted,dissolve_rect(c,0,0,30,2),point);call(c,defaulted,"pixelDissolve",3,a);unsigned changed=0;for(unsigned y=0;y<2;++y)for(unsigned x=0;x<30;++x)changed+=read(c,defaulted,x,y)==0;assert(changed==2);
    FVM_Channel *ch=0;FVM_Value value=ref(all),copy;assert(fvm_channel_create(&ch)==FVM_OK);assert(fvm_channel_send_value(c,ch,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&copy,0)==FVM_OK);dissolve_packet(other,copy.object);fvm_channel_destroy(ch);
    assert(fvm_channel_open_file("dissolve-test.bin","w",&ch)==FVM_OK);assert(fvm_channel_send_value(c,ch,&value,0)==FVM_OK);fvm_channel_destroy(ch);assert(fvm_channel_open_file("dissolve-test.bin","r",&ch)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&copy,0)==FVM_OK);fvm_channel_destroy(ch);dissolve_packet(other,copy.object);assert(std::remove("dissolve-test.bin")==0);
#ifdef OS_PORTS
    FVM_ResourceBroker *broker=0;FVM_Channel *queue=0,*network=0;FVM_Object *wrapped=0;char *uri=0;assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);assert(fvm_context_set_broker(c,broker)==FVM_OK);assert(fvm_channel_create(&queue)==FVM_OK);assert(fvm_extensions_wrap_channel(c,queue,&wrapped)==FVM_OK);assert(fvm_broker_export(c,wrapped,&uri)==FVM_OK);assert(fvm_channel_open_network(other,uri,&network)==FVM_OK);assert(fvm_channel_send_value(c,queue,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,network,&copy,0)==FVM_OK);dissolve_packet(other,copy.object);fvm_channel_release(network);fvm_broker_stop(broker);fvm_channel_release(queue);fvm_broker_release(broker);fvm_serialized_free(uri);
#endif
    FVM_Object *disposed=bitmap(c,4,3,true,0),*callback=0;assert(fvm_object_create(c,&callback)==FVM_OK);assert(fvm_gc_pin(c,callback)==FVM_OK);assert(fvm_object_define_method_rooted(c,callback,"valueOf",noise_dispose_seed,disposed,disposed)==FVM_OK);dissolve_args(a,source,rect,point);a[3]=ref(callback);assert(call(c,disposed,"pixelDissolve",6,a).number==-1);
    fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
#endif
