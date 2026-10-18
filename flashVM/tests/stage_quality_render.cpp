/* LOW affects solid/stroke/gradient/bitmap MovieClip edges; standalone paths retain AA. */
#define FVM_MOVIECLIP_GRADIENT_FIXTURE_ONLY
#include "movieclip_gradient.cpp"
#include "flashvm/stage.h"
#include "flashvm/transfer.h"
#include "../src/render/quality_internal.hpp"
static void triangle(FVM_Context *c,FVM_Object *clip)
{
    const double xy[]={0.2,0.3,7.6,1.4,2.3,7.7,0.2,0.3};FVM_Value a[2],out;
    for(unsigned i=0;i<8;i+=2){fvm_value_number(a,xy[i]);fvm_value_number(a+1,xy[i+1]);assert(fvm_object_call(c,clip,i?"lineTo":"moveTo",2,a,&out)==FVM_OK);}
    assert(fvm_object_call(c,clip,"endFill",0,0,&out)==FVM_OK);
}
static unsigned partial(const unsigned char *p)
{
    unsigned count=0;for(unsigned i=3;i<256;i+=4)if(p[i] && p[i]!=255)++count;return count;
}
static bool visible(const unsigned char *p)
{for(unsigned i=3;i<256;i+=4)if(p[i])return true;return false;}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *root;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"quality",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
    for(unsigned style=0;style<4;++style)
    {
        FVM_Object *clip=new_child(c,root,"paint",style+1);FVM_Value a[8],out;
        if(style==0){fvm_value_number(a,0xff0000);assert(fvm_object_call(c,clip,"beginFill",1,a,&out)==FVM_OK);}
        else if(style==1){arguments(c,a,false);assert(fvm_object_call(c,clip,"beginGradientFill",8,a,&out)==FVM_OK);}
        else if(style==2)
        {
            FVM_Object *image;fvm_value_number(a,1);fvm_value_number(a+1,1);fvm_value_boolean(a+2,0);fvm_value_number(a+3,0xff0000);
            assert(fvm_construct(c,"flash.display.BitmapData",4,a,&image)==FVM_OK);ref(a,image);
            assert(fvm_object_call(c,clip,"beginBitmapFill",1,a,&out)==FVM_OK);
        }
        else {fvm_value_number(a,1);fvm_value_number(a+1,0xff0000);assert(fvm_object_call(c,clip,"lineStyle",2,a,&out)==FVM_OK);}
        triangle(c,clip);
        unsigned char high[256]={0},low[256]={0},again[256]={0};FVM_RasterSurface surface={high,8,8,32};
        assert(fvm_stage_set_quality(c,FVM_STAGE_QUALITY_HIGH)==FVM_OK);
        FVM_Result r=fvm_movieclip_render(c,clip,&surface);
        if(r==FVM_UNSUPPORTED)continue;
        assert(r==FVM_OK && visible(high) && partial(high)>0);
        const unsigned char toggle[]={8,0};assert(fvm_avm1_execute(c,toggle,sizeof(toggle))==FVM_OK);
        surface.pixels=low;assert(fvm_movieclip_render(c,clip,&surface)==FVM_OK && visible(low) && !partial(low));
        assert(std::memcmp(high,low,sizeof(high))!=0);
        assert(fvm_stage_set_quality(c,FVM_STAGE_QUALITY_BEST)==FVM_OK && fvm_stage_toggle_quality(c)==FVM_OK && fvm_stage_toggle_quality(c)==FVM_OK);
        surface.pixels=again;assert(fvm_movieclip_render(c,clip,&surface)==FVM_OK && !std::memcmp(high,again,sizeof(high)));
        assert(fvm_gc_collect(c,0)==FVM_OK);
        /* Transport clip geometry, never context-wide stage policy. */
        FVM_Context *other;FVM_Channel *channel;FVM_Value packet,restored;
        assert(fvm_context_create(rt,&other)==FVM_OK && fvm_channel_create(&channel)==FVM_OK);
        assert(fvm_stage_set_quality(c,FVM_STAGE_QUALITY_LOW)==FVM_OK);ref(&packet,clip);
        assert(fvm_channel_send_value(c,channel,&packet,0)==FVM_OK && fvm_channel_receive_value(other,channel,&restored,0)==FVM_OK);
        FVM_StageQuality imported_quality;assert(fvm_stage_get_quality(other,&imported_quality)==FVM_OK && imported_quality==FVM_STAGE_QUALITY_HIGH);
        std::memset(again,0,sizeof(again));surface.pixels=again;
        assert(fvm_movieclip_render(other,restored.object,&surface)==FVM_OK && partial(again)>0);
        fvm_channel_destroy(channel);fvm_context_destroy(other);
    }
    FVM_Path *path;assert(fvm_path_create(&path)==FVM_OK);
    FVM_PathCommand command;std::memset(&command,0,sizeof(command));command.verb=FVM_PATH_MOVE;command.x[0]=0.2;command.y[0]=0.3;assert(fvm_path_add(path,&command)==FVM_OK);
    command.verb=FVM_PATH_LINE;command.x[0]=7.6;command.y[0]=1.4;assert(fvm_path_add(path,&command)==FVM_OK);command.x[0]=2.3;command.y[0]=7.7;assert(fvm_path_add(path,&command)==FVM_OK);command.verb=FVM_PATH_CLOSE;assert(fvm_path_add(path,&command)==FVM_OK);
    unsigned char pixels[256]={0};FVM_RasterSurface surface={pixels,8,8,32};FVM_RasterColor red={255,0,0,255};
    assert(fvm_stage_set_quality(c,FVM_STAGE_QUALITY_LOW)==FVM_OK);
    FVM_Result r=fvm_path_draw(path,&surface,red,0,0);assert(r==FVM_UNSUPPORTED || (r==FVM_OK && partial(pixels)>0));
    fvm_path_destroy(path);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
