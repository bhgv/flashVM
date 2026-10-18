#define FVM_NOISE_FIXTURE_ONLY
#include "bitmap_noise.cpp"
static inline void perlin_args(FVM_Value *a,double x=3,double y=5,double octaves=3,double seed=1,bool stitch=false,bool fractal=false,unsigned mask=7,bool gray=false,FVM_Object *offsets=0)
{
    fvm_value_number(a,x);fvm_value_number(a+1,y);fvm_value_number(a+2,octaves);fvm_value_number(a+3,seed);fvm_value_boolean(a+4,stitch);fvm_value_boolean(a+5,fractal);fvm_value_number(a+6,mask);fvm_value_boolean(a+7,gray);fvm_value_undefined(a+8);if(offsets)a[8]=ref(offsets);
}
static inline void perlin_pixels(FVM_Context *c,FVM_Object *image,bool fractal)
{
    const unsigned long turbulence[]={0xff000000UL,0xff126443UL,0xff144d57UL,0xff000000UL,0xff4c1b25UL,0xff3b7c21UL,0xff366f32UL,0xff445d19UL,0xff43121aUL,0xff367914UL,0xff308947UL,0xff31801fUL};
    const unsigned long summed[]={0xff7f7f7fUL,0xff824f93UL,0xff795c84UL,0xff7f7f7fUL,0xffa58673UL,0xff9c4690UL,0xff8e4886UL,0xff99767cUL,0xffa07b80UL,0xff8e4d80UL,0xff8545a3UL,0xff8b5785UL};
    for(unsigned y=0;y<3;++y)for(unsigned x=0;x<4;++x)assert(read(c,image,x,y)==(fractal?summed:turbulence)[y*4+x]);
}
static inline void perlin_packet(FVM_Context *c,FVM_Object *image)
{
    assert(fvm_gc_pin(c,image)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);FVM_Value a[9];perlin_args(a);call(c,image,"perlinNoise",8,a);perlin_pixels(c,image,false);
}
#ifndef FVM_PERLIN_FIXTURE_ONLY
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);FVM_Object *image=bitmap(c,4,3,true,0);FVM_Value a[9];perlin_args(a);unsigned long random=c->random_state;call(c,image,"perlinNoise",8,a);perlin_pixels(c,image,false);assert(c->random_state==random);
    perlin_args(a,3,5,3,1,false,true);call(c,image,"perlinNoise",8,a);perlin_pixels(c,image,true);perlin_args(a,3,5,3,0,false,true);call(c,image,"perlinNoise",8,a);perlin_pixels(c,image,true);
    perlin_args(a,0,0,3,1,false,true);call(c,image,"perlinNoise",8,a);for(unsigned y=0;y<3;++y)for(unsigned x=0;x<4;++x)assert(read(c,image,x,y)==0xff7f7f7fUL);
    perlin_args(a,3,5,0,1,false,false);call(c,image,"perlinNoise",8,a);assert(read(c,image,2,1)==0xff000000UL);perlin_args(a,3,5,0,1,false,true,15);call(c,image,"perlinNoise",8,a);assert(read(c,image,2,1)==0x7f7f7f7fUL);
    FVM_Object *red=bitmap(c,4,3,true,0),*green=bitmap(c,4,3,true,0);perlin_args(a,3,5,3,1,false,false,1);call(c,red,"perlinNoise",8,a);perlin_args(a,3,5,3,1,false,false,2);call(c,green,"perlinNoise",8,a);for(unsigned y=0;y<3;++y)for(unsigned x=0;x<4;++x)assert(((unsigned long)read(c,red,x,y)>>16 & 255)==((unsigned long)read(c,green,x,y)>>8 & 255));
    perlin_args(a,3,5,3,1,false,true,0,true);call(c,image,"perlinNoise",8,a);for(unsigned y=0;y<3;++y)for(unsigned x=0;x<4;++x){unsigned long v=(unsigned long)read(c,image,x,y);assert((v&255)==((v>>8)&255) && (v&255)==((v>>16)&255));}
    FVM_Object *opaque=bitmap(c,4,3,false,0);perlin_args(a,3,5,3,1,false,true,15);call(c,opaque,"perlinNoise",8,a);for(unsigned y=0;y<3;++y)for(unsigned x=0;x<4;++x)assert(((unsigned long)read(c,opaque,x,y)>>24)==255);
    FVM_Object *offsets=0;FVM_Value length;fvm_value_number(&length,3);assert(fvm_construct(c,"Array",1,&length,&offsets)==FVM_OK);assert(fvm_gc_pin(c,offsets)==FVM_OK);for(unsigned i=0;i<3;++i){FVM_Value point=ref(hit_point(c,1,0));assert(fvm_table_set_index(c,offsets,i,&point)==FVM_OK);}
    perlin_args(a,3,5,3,1,false,false,7,false,offsets);call(c,image,"perlinNoise",9,a);perlin_args(a);call(c,opaque,"perlinNoise",8,a);for(unsigned y=0;y<3;++y)for(unsigned x=0;x<3;++x)assert(read(c,image,x,y)==read(c,opaque,x+1,y));
    for(unsigned i=0;i<3;++i){FVM_Value point=ref(hit_point(c,4,0));assert(fvm_table_set_index(c,offsets,i,&point)==FVM_OK);}perlin_args(a,3,3,3,1,true,false);call(c,opaque,"perlinNoise",8,a);perlin_args(a,3,3,3,1,true,false,7,false,offsets);call(c,image,"perlinNoise",9,a);for(unsigned y=0;y<3;++y)assert(read(c,image,0,y)==read(c,opaque,0,y));
    unsigned long before=(unsigned long)read(c,image,1,1),limit=rt->config.memory_limit_bytes;rt->config.memory_limit_bytes=48;perlin_args(a);FVM_Value out;assert(fvm_object_call(c,image,"perlinNoise",8,a,&out)==FVM_OUT_OF_MEMORY);rt->config.memory_limit_bytes=limit;assert(read(c,image,1,1)==before);fvm_value_number(a+2,17);assert(fvm_object_call(c,image,"perlinNoise",8,a,&out)==FVM_INVALID_ARGUMENT);assert(read(c,image,1,1)==before);
    FVM_Channel *ch=0;FVM_Value value=ref(image),copy;assert(fvm_channel_create(&ch)==FVM_OK);assert(fvm_channel_send_value(c,ch,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&copy,0)==FVM_OK);perlin_packet(other,copy.object);fvm_channel_destroy(ch);
    FVM_Result opened=fvm_channel_open_file("perlin-test.bin","w",&ch);if(opened==FVM_OK){assert(fvm_channel_send_value(c,ch,&value,0)==FVM_OK);fvm_channel_destroy(ch);assert(fvm_channel_open_file("perlin-test.bin","r",&ch)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&copy,0)==FVM_OK);fvm_channel_destroy(ch);perlin_packet(other,copy.object);assert(std::remove("perlin-test.bin")==0);}else assert(opened==FVM_UNSUPPORTED);
#ifdef OS_PORTS
    FVM_ResourceBroker *broker=0;FVM_Channel *queue=0,*network=0;FVM_Object *wrapped=0;char *uri=0;assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);assert(fvm_context_set_broker(c,broker)==FVM_OK);assert(fvm_channel_create(&queue)==FVM_OK);assert(fvm_extensions_wrap_channel(c,queue,&wrapped)==FVM_OK);assert(fvm_broker_export(c,wrapped,&uri)==FVM_OK);assert(fvm_channel_open_network(other,uri,&network)==FVM_OK);assert(fvm_channel_send_value(c,queue,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,network,&copy,0)==FVM_OK);perlin_packet(other,copy.object);fvm_channel_release(network);fvm_broker_stop(broker);fvm_channel_release(queue);fvm_broker_release(broker);fvm_serialized_free(uri);
#endif
    FVM_Object *disposed=bitmap(c,4,3,true,0),*point=0;assert(fvm_object_create(c,&point)==FVM_OK);assert(fvm_gc_pin(c,point)==FVM_OK);set(c,point,"y",0);assert(fvm_object_define_accessor_rooted(c,point,"x",hit_dispose,0,disposed,disposed)==FVM_OK);value=ref(point);assert(fvm_table_set_index(c,offsets,0,&value)==FVM_OK);perlin_args(a,3,5,3,1,false,false,7,false,offsets);assert(call(c,disposed,"perlinNoise",9,a).number==-1);
    fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
#endif
