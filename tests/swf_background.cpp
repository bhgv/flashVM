/* SetBackgroundColor is a replayable, graph-transported timeline control with stage-wide first win. */
#include "flashvm/movieclip.h"
#include "flashvm/stage.h"
#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cassert>
typedef fvm_detail::vector<unsigned char> Bytes;
static void word(Bytes &b,unsigned n){b.push_back((unsigned char)n);b.push_back((unsigned char)(n>>8));}
static void tag(Bytes &b,unsigned code,const Bytes &p){assert(p.size()<63);word(b,(code<<6)|(unsigned)p.size());for(size_t i=0;i<p.size();++i)b.push_back(p[i]);}
static void rgb(Bytes &tags,unsigned r,unsigned g,unsigned b){Bytes p;p.push_back((unsigned char)r);p.push_back((unsigned char)g);p.push_back((unsigned char)b);tag(tags,9,p);}
static Bytes movie(const Bytes &tags)
{
    Bytes b;b.push_back('F');b.push_back('W');b.push_back('S');b.push_back(8);unsigned long n=(unsigned long)tags.size()+14;for(unsigned i=0;i<4;++i){b.push_back((unsigned char)n);n>>=8;}b.push_back(8);b.push_back(0);word(b,12*256);word(b,2);for(size_t i=0;i<tags.size();++i)b.push_back(tags[i]);return b;
}
static void seek(FVM_Context *c,FVM_Object *clip,unsigned frame)
{ FVM_Value a,out;fvm_value_number(&a,frame);assert(fvm_object_call(c,clip,"gotoAndStop",1,&a,&out)==FVM_OK);assert(fvm_movieclip_advance(c,clip,1)==FVM_OK); }
static void color(FVM_Context *c,unsigned r,unsigned g,unsigned b,unsigned a)
{ FVM_RasterColor out;assert(fvm_stage_get_background(c,&out)==FVM_OK && out.r==r && out.g==g && out.b==b && out.a==a); }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);assert(fvm_standard_install(other)==FVM_OK);
    Bytes tags,empty;rgb(tags,0x12,0x34,0x56);tag(tags,1,empty);rgb(tags,0xaa,0xbb,0xcc);tag(tags,1,empty);tag(tags,0,empty);Bytes swf=movie(tags);
    FVM_Object *clip=0;assert(fvm_movieclip_load_swf(c,swf.begin(),swf.size(),"root",&clip)==FVM_OK);assert(fvm_gc_pin(c,clip)==FVM_OK);
    FVM_RasterColor out={1,2,3,4};assert(fvm_stage_get_background(c,&out)==FVM_NOT_FOUND && out.r==1);
    assert(fvm_movieclip_advance(c,clip,1)==FVM_OK);color(c,0x12,0x34,0x56,255);
    assert(fvm_movieclip_advance(c,clip,1)==FVM_OK);color(c,0x12,0x34,0x56,255);
    assert(fvm_stage_clear_background(c)==FVM_OK);seek(c,clip,2);color(c,0x12,0x34,0x56,255);
    FVM_RasterColor host={1,2,3,77};assert(fvm_stage_set_background(c,host)==FVM_OK);seek(c,clip,1);color(c,1,2,3,77);
    FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK);FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=clip;
    assert(fvm_channel_send_value(c,channel,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,channel,&value,0)==FVM_OK);FVM_Object *copy=value.object;assert(fvm_gc_pin(other,copy)==FVM_OK);
    assert(fvm_stage_get_background(other,&out)==FVM_NOT_FOUND);seek(other,copy,1);color(other,0x12,0x34,0x56,255);
    assert(fvm_gc_collect(other,0)==FVM_OK);color(other,0x12,0x34,0x56,255);
    FVM_Object *state=copy->resource_result.object;FVM_Object *frames=state->properties["frames"].object;FVM_Object *frame=frames->indexed[1].object;FVM_Object *controls=frame->properties["controls"].object;FVM_Object *control=controls->indexed[0].object;
    fvm_value_number(&control->properties["background"],16777216.0);FVM_Value arg,result;fvm_value_number(&arg,1);assert(fvm_object_call(other,copy,"gotoAndStop",1,&arg,&result)==FVM_OK);assert(fvm_movieclip_advance(other,copy,1)==FVM_INVALID_ARGUMENT);color(other,0x12,0x34,0x56,255);
    Bytes malformed,short_rgb;short_rgb.push_back(1);short_rgb.push_back(2);tag(malformed,9,short_rgb);tag(malformed,1,empty);tag(malformed,1,empty);tag(malformed,0,empty);Bytes bad=movie(malformed);FVM_Object *rejected=clip;assert(fvm_movieclip_load_swf(c,bad.begin(),bad.size(),"bad",&rejected)==FVM_INVALID_ARGUMENT && !rejected);color(c,1,2,3,77);
    fvm_channel_destroy(channel);fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
