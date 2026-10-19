/* Independent pixel oracle for overlapping signed-depth children, viewport and transported geometry. */
#include "movieclip_tick_order_fixture.hpp"
#include "flashvm/raster.h"
#include "flashvm/transfer.h"
#include "flashvm/channel.h"
#include <cstring>
static void rectangle(FVM_Context *c,FVM_Object *clip,unsigned rgb)
{
    FVM_Value args[2],out;fvm_value_number(args,rgb);
    assert(fvm_object_call(c,clip,"beginFill",1,args,&out)==FVM_OK);
    const unsigned xy[]={1,1,5,1,5,5,1,5,1,1};
    for(unsigned i=0;i<10;i+=2)
    {fvm_value_number(args,xy[i]);fvm_value_number(args+1,xy[i+1]);assert(fvm_object_call(c,clip,i?"lineTo":"moveTo",2,args,&out)==FVM_OK);}
    assert(fvm_object_call(c,clip,"endFill",0,0,&out)==FVM_OK);
}
static void check(FVM_Context *c,FVM_Object *root,unsigned red,unsigned green,unsigned blue)
{
    unsigned char pixels[256]={0};FVM_RasterSurface surface={pixels,8,8,32};
    FVM_Result result=fvm_movieclip_render(c,root,&surface);
    if(result==FVM_UNSUPPORTED)return;
    assert(result==FVM_OK);
    unsigned char *p=pixels+4*(3*8+3);
    assert(p[0]==red && p[1]==green && p[2]==blue && p[3]==255);
    FVM_Value args[4],out;fvm_value_number(args,8);fvm_value_number(args+1,8);
    fvm_value_boolean(args+2,1);fvm_value_number(args+3,0);FVM_Object *bitmap=0;
    assert(fvm_construct(c,"flash.display.BitmapData",4,args,&bitmap)==FVM_OK);
    args[0]=tick_object(root);assert(fvm_object_call(c,bitmap,"draw",1,args,&out)==FVM_OK);
    fvm_value_number(args,3);fvm_value_number(args+1,3);
    assert(fvm_object_call(c,bitmap,"getPixel32",2,args,&out)==FVM_OK);
    assert(out.type==FVM_VALUE_NUMBER && out.number==double(0xff000000UL | (red<<16) | (green<<8) | blue)-4294967296.0);
    FVM_Affine view={1,0,0,1,2,1};std::memset(pixels,0,sizeof(pixels));
    assert(fvm_movieclip_render_view(c,root,&surface,&view)==FVM_OK);
    p=pixels+4*(4*8+5);assert(p[0]==red && p[1]==green && p[2]==blue && p[3]==255);
    assert(!pixels[4*(2*8+1)+3]);
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)if(!version || version>=5)
    {
        FVM_Context *c=tick_context(rt,version),*other=tick_context(rt,version);FVM_Object *root=tick_root(c);
        FVM_Object *high=ordered_child(c,root,"high",10);rectangle(c,high,0x0000ff);
        rectangle(c,ordered_child(c,root,"low",-5),0xff0000);
        rectangle(c,ordered_child(c,root,"middle",2),0x00ff00);
        check(c,root,0,0,255);
        FVM_Value arg,out;fvm_value_number(&arg,-10);
        assert(fvm_object_call(c,high,"swapDepths",1,&arg,&out)==FVM_OK);
        check(c,root,0,255,0);
        FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK);
        arg=tick_object(root);assert(fvm_channel_send_value(c,channel,&arg,0)==FVM_OK);
        assert(fvm_channel_receive_value(other,channel,&out,0)==FVM_OK && out.type==FVM_VALUE_OBJECT);
        assert(fvm_gc_pin(other,out.object)==FVM_OK);fvm_context_destroy(c);
        assert(fvm_gc_collect(other,0)==FVM_OK);check(other,out.object,0,255,0);
        fvm_channel_destroy(channel);fvm_context_destroy(other);
    }
    fvm_runtime_destroy(rt);
}
