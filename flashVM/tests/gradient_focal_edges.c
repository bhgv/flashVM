/* Known circles around the focal point provide independent off-axis ratios without solving a quadratic. */
#define main gradient_reference_main
#include "gradient.c"
#undef main
#include <float.h>
int main(void)
{
    FVM_Path *path=box();FVM_Gradient g;FVM_GradientStop stops[2];FVM_RasterSurface surface;unsigned char pixels[1024],saved[1024];const double focals[]={-0.75,0.75,-0.999999,0.999999};const double directions[4][2]={{1,0},{-1,0},{0,1},{0.6,0.8}};const double ratios[]={0.25,0.5,0.75,1.25,1.5,1.75};const unsigned char expected[3][6]={{191,128,64,0,0,0},{191,128,64,64,128,191},{191,128,64,191,128,64}};unsigned f,d,t,mode;FVM_Result result;
    assert(fvm_gradient_init(&g)==FVM_OK);g.type=FVM_GRADIENT_RADIAL;g.stops=stops;g.count=2;stop(stops,0,255,0,0,255);stop(stops+1,1,0,0,255,255);surface.pixels=pixels;surface.width=surface.height=16;surface.stride=64;
    for(f=0;f<4;++f)for(d=0;d<4;++d)for(t=0;t<6;++t)for(mode=0;mode<3;++mode){double x=focals[f]+ratios[t]*(directions[d][0]-focals[f]),y=ratios[t]*directions[d][1];g.matrix.x0=8.5-x;g.matrix.y0=8.5-y;memset(pixels,0,sizeof(pixels));result=fvm_path_draw_gradient_focal(path,&surface,&g,0,mode,0,focals[f]);assert(result==FVM_OK || result==FVM_UNSUPPORTED);if(result==FVM_OK)assert(near(pixels[4*(8*16+8)],expected[mode][t]) && pixels[4*(8*16+8)+3]==255);}
    /* A precisely representable center sample stays finite even at the closest interior values. */
    g.matrix.x0=0.5;g.matrix.y0=8.5;memset(pixels,0,sizeof(pixels));result=fvm_path_draw_gradient_focal(path,&surface,&g,0,0,0,1-DBL_EPSILON);if(result==FVM_OK)assert(near(pixels[4*(8*16)],128));memset(pixels,0,sizeof(pixels));result=fvm_path_draw_gradient_focal(path,&surface,&g,0,0,0,-1+DBL_EPSILON);if(result==FVM_OK)assert(near(pixels[4*(8*16)],128));
    memset(pixels,37,sizeof(pixels));memcpy(saved,pixels,sizeof(pixels));assert(fvm_path_draw_gradient_focal(path,&surface,&g,0,0,0,HUGE_VAL)==FVM_INVALID_ARGUMENT && !memcmp(pixels,saved,sizeof(pixels)));assert(fvm_path_stroke_gradient_focal(path,&surface,&g,0,0,0,0.5)==FVM_INVALID_ARGUMENT);fvm_path_destroy(path);return 0;
}
