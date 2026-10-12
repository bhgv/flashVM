#define FVM_BITMAP_FILL_FIXTURE_ONLY
#include "movieclip_bitmap_fill.cpp"
#include "flashvm/property.h"
static inline FVM_Result draw_result(FVM_Context *c,FVM_Object *dest,FVM_Object *source,FVM_Object *matrix=0,FVM_Object *color=0,unsigned mode=1,FVM_Object *clip=0,bool smooth=false)
{
    FVM_Value a[6],out;a[0]=ref(source);for(unsigned i=1;i<6;++i)fvm_value_undefined(a+i);
    if(matrix)a[1]=ref(matrix);if(color)a[2]=ref(color);fvm_value_number(a+3,mode);if(clip)a[4]=ref(clip);fvm_value_boolean(a+5,smooth);
    return fvm_object_call(c,dest,"draw",6,a,&out);
}
static inline FVM_Object *rectangle(FVM_Context *c,const double *values){return make(c,"flash.geom.Rectangle",4,values);}
static inline void fill_color(FVM_Context *c,FVM_Object *o,unsigned long color){FVM_Value a;fvm_value_number(&a,(double)color);call(c,o,"beginFill",1,&a);geometry(c,o,true);}
static inline void set_number(FVM_Context *c,FVM_Object *o,const char *name,double number){FVM_Value v;fvm_value_number(&v,number);assert(fvm_object_set(c,o,name,&v)==FVM_OK);}
static inline void check_draw(FVM_Context *c,FVM_Object *source)
{
    FVM_Object *dest=bitmap(c,8,8,true,0);FVM_Result r=draw_result(c,dest,source);if(r==FVM_UNSUPPORTED)return;assert(r==FVM_OK);assert(read(c,dest,0,0)==0xffff0000UL);assert(read(c,dest,2,0)==0xff00ff00UL);
}
static FVM_Result dispose_target(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *u)
{
    FVM_Value ignored;assert(fvm_object_call(c,(FVM_Object *)u,"dispose",0,0,&ignored)==FVM_OK);fvm_value_number(out,2);return FVM_OK;
}
#ifndef FVM_BITMAP_DRAW_FIXTURE_ONLY
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    FVM_Object *source=bitmap(c,2,1,true,0xffff0000UL);write(c,source,1,0,0xff00ff00UL);FVM_Object *dest=bitmap(c,4,2,true,0);
    double scale_values[]={2,0,0,2,0,0};FVM_Object *scale=make(c,"flash.geom.Matrix",6,scale_values);
    FVM_Result rendered=draw_result(c,dest,source,scale);assert(rendered==FVM_OK || rendered==FVM_UNSUPPORTED);
    if(rendered==FVM_OK){assert(read(c,dest,1,0)==0xffff0000UL);assert(read(c,dest,2,0)==0xff00ff00UL);
        assert(draw_result(c,dest,source,scale,0,1,0,true)==FVM_OK);unsigned long p=(unsigned long)read(c,dest,1,0);assert(((p>>16)&255)>=190 && ((p>>16)&255)<=192 && ((p>>8)&255)>=63 && ((p>>8)&255)<=65);}
    double reflection_values[]={-1,0,0,1,2,0};FVM_Object *reflection=make(c,"flash.geom.Matrix",6,reflection_values);
    dest=bitmap(c,2,1,false,0);if(draw_result(c,dest,source,reflection)==FVM_OK){assert(read(c,dest,0,0)==0xff00ff00UL);assert(read(c,dest,1,0)==0xffff0000UL);}
    double clip_values[]={1,0,1,1};FVM_Object *clip=rectangle(c,clip_values);dest=bitmap(c,2,1,false,0xff0000ffUL);
    if(draw_result(c,dest,source,0,0,1,clip)==FVM_OK){assert(read(c,dest,0,0)==0xff0000ffUL);assert(read(c,dest,1,0)==0xff00ff00UL);}
    FVM_Object *alias=bitmap(c,4,1,false,0xffff0000UL);write(c,alias,1,0,0xff00ff00UL);write(c,alias,2,0,0xff0000ffUL);write(c,alias,3,0,0xffffffffUL);
    double translate_values[]={1,0,0,1,1,0};FVM_Object *translate=make(c,"flash.geom.Matrix",6,translate_values);
    if(draw_result(c,alias,alias,translate)==FVM_OK){assert(read(c,alias,0,0)==0xffff0000UL);assert(read(c,alias,1,0)==0xffff0000UL);assert(read(c,alias,2,0)==0xff00ff00UL);assert(read(c,alias,3,0)==0xff0000ffUL);}
    FVM_Object *blend_source=bitmap(c,1,1,true,0x80c86432UL);
    const unsigned expected[][3]={{150,150,50},{150,150,50},{89,139,30},{161,211,70},{150,200,50},{100,150,50},{100,150,25},{178,228,75},{50,150,25},{128,127,128},{0,0,0},{0,0,0},{129,194,35},{144,178,35}};
    for(unsigned mode=1;mode<=14;++mode){FVM_Object *target=bitmap(c,1,1,false,0xff64c832UL);FVM_Result r=draw_result(c,target,blend_source,0,0,mode);
        if(mode==11 || mode==12){assert(r==(rendered==FVM_OK?FVM_OK:FVM_UNSUPPORTED));assert(read(c,target,0,0)==0xff64c832UL);continue;}
        if(rendered==FVM_UNSUPPORTED){assert(r==FVM_UNSUPPORTED);continue;}assert(r==FVM_OK);unsigned long value=(unsigned long)read(c,target,0,0);
        for(unsigned channel=0;channel<3;++channel){unsigned actual=(unsigned)((value>>(16-channel*8))&255);assert(actual+1>=expected[mode-1][channel] && actual<=expected[mode-1][channel]+1);}assert((value>>24)==255);}
    FVM_Object *root=0;assert(fvm_movieclip_create(c,"source",1,&root)==FVM_OK);assert(fvm_gc_pin(c,root)==FVM_OK);fill_color(c,root,0xff0000UL);
    FVM_Object *nested=child(c,root,"child",1);fill_color(c,nested,0x00ff00UL);set_number(c,nested,"_x",2);
    set_number(c,root,"_x",100);set_number(c,root,"_y",100);set_number(c,root,"_xscale",200);set_number(c,root,"_alpha",0);set_number(c,root,"_visible",0);check_draw(c,root);
    FVM_Value color_args[8];const double color_values[]={0,1,0,1,0,0,255,0};for(unsigned i=0;i<8;++i)fvm_value_number(color_args+i,color_values[i]);
    FVM_Object *color=0;assert(fvm_construct(c,"flash.geom.ColorTransform",8,color_args,&color)==FVM_OK);assert(fvm_gc_pin(c,color)==FVM_OK);
    FVM_Object *colored=bitmap(c,8,8,true,0);double move_values[]={1,0,0,1,2,0};FVM_Object *move=make(c,"flash.geom.Matrix",6,move_values);
    if(draw_result(c,colored,root,move,color)==FVM_OK){assert(read(c,colored,0,0)==0);assert(read(c,colored,2,0)==0xff0000ffUL);assert(read(c,colored,4,0)==0xff00ffffUL);}
    FVM_Object *budget_target=bitmap(c,8,8,true,0xff123456UL);unsigned long previous_limit=rt->config.memory_limit_bytes;rt->config.memory_limit_bytes=32;
    assert(draw_result(c,budget_target,source)==FVM_OUT_OF_MEMORY);rt->config.memory_limit_bytes=previous_limit;assert(read(c,budget_target,0,0)==0xff123456UL);
    FVM_Value v=ref(root),out;FVM_Channel *ch=0;assert(fvm_channel_create(&ch)==FVM_OK);assert(fvm_channel_send_value(c,ch,&v,0)==FVM_OK);
    assert(fvm_channel_receive_value(other,ch,&out,0)==FVM_OK);assert(fvm_gc_pin(other,out.object)==FVM_OK);assert(fvm_gc_collect(other,0)==FVM_OK);
    /* Transport the destination class too; receiver does not install globals. */
    FVM_Object *target=bitmap(c,8,8,true,0);v=ref(target);assert(fvm_channel_send_value(c,ch,&v,0)==FVM_OK);FVM_Value copied_target;assert(fvm_channel_receive_value(other,ch,&copied_target,0)==FVM_OK);
    assert(fvm_gc_pin(other,copied_target.object)==FVM_OK);if(draw_result(other,copied_target.object,out.object)==FVM_OK){assert(read(other,copied_target.object,0,0)==0xffff0000UL);assert(read(other,copied_target.object,2,0)==0xff00ff00UL);}fvm_channel_destroy(ch);
    double singular_values[]={0,0,0,0,0,0};FVM_Object *singular=make(c,"flash.geom.Matrix",6,singular_values);assert(draw_result(c,dest,source,singular)==FVM_OK);
    FVM_Value args[4]={ref(source),ref(scale),ref(scale),ref(scale)},invalid_out;assert(fvm_object_call(c,dest,"draw",4,args,&invalid_out)==FVM_INVALID_ARGUMENT);
    if(rendered==FVM_OK){FVM_Object *disposed=bitmap(c,8,8,true,0);int removed=0;assert(fvm_object_delete(c,nested,"_x",&removed)==FVM_OK && removed);assert(fvm_object_define_accessor_rooted(c,nested,"_x",dispose_target,0,disposed,disposed)==FVM_OK);
        assert(draw_result(c,disposed,root)==FVM_INVALID_ARGUMENT);assert(fvm_bitmap_pixels(disposed)->disposed);}
    fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
#endif
