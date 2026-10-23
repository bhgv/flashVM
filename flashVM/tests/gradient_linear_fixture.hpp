/* Shared script linearRGB arguments and separately constructed C89 paint comparison. */
#ifndef FVM_TEST_GRADIENT_LINEAR_FIXTURE_HPP
#define FVM_TEST_GRADIENT_LINEAR_FIXTURE_HPP
#include "line_gradient_fixture.hpp"
static void linear_arguments(FVM_Context *c,FVM_Value *args,unsigned mode,bool radial=false)
{spread_arguments(c,args,mode,radial);assert(fvm_value_string(args+6,"linearRGB")==FVM_OK);}
static void linear_style(FVM_Context *c,FVM_Object *clip,unsigned mode,bool radial=false,bool stroke=false)
{FVM_Value args[8],out;linear_arguments(c,args,mode,radial);if(stroke){FVM_Value width;fvm_value_number(&width,4);assert(fvm_object_call(c,clip,"lineStyle",1,&width,&out)==FVM_OK);}assert(fvm_object_call(c,clip,stroke?"lineGradientStyle":"beginGradientFill",8,args,&out)==FVM_OK);}
static FVM_Result linear_pixels(FVM_Context *c,FVM_Object *clip,unsigned mode,bool radial=false,bool stroke=false,unsigned interpolation=1)
{
    unsigned char actual[1024]={0},expected[1024]={0};FVM_RasterSurface dst={actual,16,16,64},oracle={expected,16,16,64};FVM_Result result=fvm_movieclip_render(c,clip,&dst);if(result==FVM_UNSUPPORTED)return result;assert(result==FVM_OK);FVM_Path *path=0;FVM_PathCommand command;assert(fvm_path_create(&path)==FVM_OK);std::memset(&command,0,sizeof(command));const double xy[]={0,0,16,0,16,16,0,16};if(stroke){command.verb=FVM_PATH_MOVE;command.x[0]=1;command.y[0]=8;assert(fvm_path_add(path,&command)==FVM_OK);command.verb=FVM_PATH_LINE;command.x[0]=15;assert(fvm_path_add(path,&command)==FVM_OK);}else{for(unsigned i=0;i<8;i+=2){command.verb=i?FVM_PATH_LINE:FVM_PATH_MOVE;command.x[0]=xy[i];command.y[0]=xy[i+1];assert(fvm_path_add(path,&command)==FVM_OK);}command.verb=FVM_PATH_CLOSE;assert(fvm_path_add(path,&command)==FVM_OK);}
    FVM_Gradient gradient;FVM_GradientStop stops[2];assert(fvm_gradient_init(&gradient)==FVM_OK);stops[0].ratio=0;stops[0].color.r=stops[0].color.a=255;stops[0].color.g=stops[0].color.b=0;stops[1].ratio=1;stops[1].color.r=stops[1].color.g=0;stops[1].color.b=stops[1].color.a=255;gradient.stops=stops;gradient.count=2;gradient.type=radial?FVM_GRADIENT_RADIAL:FVM_GRADIENT_LINEAR;gradient.matrix.xx=gradient.matrix.yy=2;gradient.matrix.x0=4.5;gradient.matrix.y0=8.5;assert((stroke?fvm_path_stroke_gradient_interpolated(path,&oracle,&gradient,4,mode,interpolation):fvm_path_draw_gradient_interpolated(path,&oracle,&gradient,0,mode,interpolation))==FVM_OK);fvm_path_destroy(path);assert(!std::memcmp(actual,expected,sizeof(actual)));return result;
}
#endif
