/* Shared script-gradient arguments and differential comparison against the independently tested C89 sampler. */
#ifndef FVM_TEST_GRADIENT_SPREAD_FIXTURE_HPP
#define FVM_TEST_GRADIENT_SPREAD_FIXTURE_HPP
#define FVM_MOVIECLIP_GRADIENT_FIXTURE_ONLY
#include "movieclip_gradient.cpp"
static void spread_arguments(FVM_Context *c,FVM_Value *args,unsigned mode,bool radial=false)
{arguments(c,args,true);set(c,args[4].object,"w",4);set(c,args[4].object,"h",4);set(c,args[4].object,"x",2.5);set(c,args[4].object,"y",6.5);assert(fvm_value_string(args,radial?"radial":"linear")==FVM_OK && fvm_value_string(args+5,mode==1?"reflect":mode==2?"repeat":"pad")==FVM_OK);}
static FVM_Result spread_pixels(FVM_Context *c,FVM_Object *clip,unsigned mode,bool radial=false)
{
    unsigned char actual[1024],expected[1024];std::memset(actual,0,sizeof(actual));std::memset(expected,0,sizeof(expected));FVM_RasterSurface surface={actual,16,16,64},oracle={expected,16,16,64};FVM_Result result=fvm_movieclip_render(c,clip,&surface);if(result==FVM_UNSUPPORTED)return result;assert(result==FVM_OK);
    FVM_Path *path=0;assert(fvm_path_create(&path)==FVM_OK);FVM_PathCommand command;std::memset(&command,0,sizeof(command));const double xy[]={0,0,16,0,16,16,0,16};for(unsigned i=0;i<8;i+=2){command.verb=i?FVM_PATH_LINE:FVM_PATH_MOVE;command.x[0]=xy[i];command.y[0]=xy[i+1];assert(fvm_path_add(path,&command)==FVM_OK);}command.verb=FVM_PATH_CLOSE;assert(fvm_path_add(path,&command)==FVM_OK);
    FVM_Gradient gradient;assert(fvm_gradient_init(&gradient)==FVM_OK);FVM_GradientStop stops[2];stops[0].ratio=0;stops[0].color.r=255;stops[0].color.g=stops[0].color.b=0;stops[0].color.a=255;stops[1].ratio=1;stops[1].color.r=stops[1].color.g=0;stops[1].color.b=stops[1].color.a=255;gradient.stops=stops;gradient.count=2;gradient.type=radial?FVM_GRADIENT_RADIAL:FVM_GRADIENT_LINEAR;gradient.matrix.xx=gradient.matrix.yy=2;gradient.matrix.x0=4.5;gradient.matrix.y0=8.5;
    assert(fvm_path_draw_gradient_spread(path,&oracle,&gradient,0,(FVM_GradientSpread)mode)==FVM_OK);fvm_path_destroy(path);assert(!std::memcmp(actual,expected,sizeof(actual)));return FVM_OK;
}
#endif
