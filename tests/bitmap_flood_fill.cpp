#define FVM_BITMAP_FIXTURE_ONLY
#include "standard_bitmap.cpp"
#include <limits>
static inline FVM_Result flood(FVM_Context *c,FVM_Object *image,double x,double y,double color)
{
    FVM_Value args[3],out;fvm_value_number(args,x);fvm_value_number(args+1,y);fvm_value_number(args+2,color);return fvm_object_call(c,image,"floodFill",3,args,&out);
}
static FVM_Result dispose_in_valueof(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *u)
{
    FVM_Value ignored;assert(fvm_object_call(c,(FVM_Object *)u,"dispose",0,0,&ignored)==FVM_OK);fvm_value_number(out,0);return FVM_OK;
}
static inline void flood_received(FVM_Context *c,FVM_Object *image)
{
    assert(fvm_gc_pin(c,image)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);assert(flood(c,image,0,0,0xffaabbccUL)==FVM_OK);assert(read(c,image,0,0)==0xffaabbccUL);
}
#ifndef FVM_FLOOD_FILL_FIXTURE_ONLY
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    FVM_Object *image=bitmap(c,5,5,false,0xff0000ffUL);for(unsigned y=1;y<4;++y)for(unsigned x=1;x<4;++x)write(c,image,x,y,0xffff0000UL);
    write(c,image,0,0,0xffff0000UL);write(c,image,2,2,0xff00ff00UL);assert(flood(c,image,1,1,0x80112233UL)==FVM_OK);
    for(unsigned y=1;y<4;++y)for(unsigned x=1;x<4;++x)assert(read(c,image,x,y)==(x==2 && y==2?0xff00ff00UL:0xff112233UL));
    assert(read(c,image,0,0)==0xffff0000UL);assert(read(c,image,4,4)==0xff0000ffUL);
    FVM_Object *alpha=bitmap(c,4,1,true,0x40112233UL);write(c,alpha,2,0,0x80112233UL);
    assert(flood(c,alpha,0,0,0x00010203UL)==FVM_OK);assert(read(c,alpha,0,0)==0x010203UL && read(c,alpha,1,0)==0x010203UL);
    assert(read(c,alpha,2,0)==0x80112233UL && read(c,alpha,3,0)==0x40112233UL);
    assert(flood(c,alpha,-.8,0,-1)==FVM_OK);assert(read(c,alpha,0,0)==0xffffffffUL && read(c,alpha,1,0)==0xffffffffUL);
    assert(flood(c,alpha,1e100,0,0)==FVM_OK && read(c,alpha,0,0)==0xffffffffUL);
    assert(flood(c,alpha,0,0,std::numeric_limits<double>::infinity())==FVM_INVALID_ARGUMENT && read(c,alpha,0,0)==0xffffffffUL);
    FVM_Object *large=bitmap(c,128,128,true,0xff224466UL);unsigned long original_limit=rt->config.memory_limit_bytes;rt->config.memory_limit_bytes=70000;
    assert(flood(c,large,64,64,0xff112233UL)==FVM_OK);assert(read(c,large,0,0)==0xff112233UL && read(c,large,127,127)==0xff112233UL);
    rt->config.memory_limit_bytes=67585;assert(flood(c,large,64,64,0xff445566UL)==FVM_OUT_OF_MEMORY && read(c,large,64,64)==0xff112233UL);
    rt->config.memory_limit_bytes=1;assert(flood(c,large,64,64,0xff112233UL)==FVM_OK);rt->config.memory_limit_bytes=original_limit;
    FVM_Channel *ch=0;FVM_Value value=ref(image),out;assert(fvm_channel_create(&ch)==FVM_OK);assert(fvm_channel_send_value(c,ch,&value,0)==FVM_OK);
    assert(fvm_channel_receive_value(other,ch,&out,0)==FVM_OK);flood_received(other,out.object);fvm_channel_destroy(ch);
    FVM_Object *disposed=bitmap(c,2,2,true,0),*coercion=0;assert(fvm_object_create(c,&coercion)==FVM_OK);assert(fvm_gc_pin(c,coercion)==FVM_OK);
    assert(fvm_object_define_method_rooted(c,coercion,"valueOf",dispose_in_valueof,disposed,disposed)==FVM_OK);
    FVM_Value args[3]={ref(coercion),ref(coercion),ref(coercion)},ignored;fvm_value_number(args+1,0);fvm_value_number(args+2,0xff010203UL);
    assert(fvm_object_call(c,disposed,"floodFill",3,args,&ignored)==FVM_INVALID_ARGUMENT);assert(fvm_bitmap_pixels(disposed)->disposed);
    fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
#endif
