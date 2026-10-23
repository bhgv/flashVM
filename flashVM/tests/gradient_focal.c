/* Independent horizontal ray ratios and zero-focal byte identity exercise both public focal entries. */
#define main gradient_reference_main
#include "gradient.c"
#undef main
int main(void)
{
    FVM_Path *path=box();FVM_Gradient g;FVM_GradientStop stops[2];FVM_RasterSurface surface;unsigned char pixels[1024],legacy[1024],saved[1024];FVM_Result result;unsigned sign,mode,interpolation,stroke,x;const unsigned char red[2][16]={{0,128,255,213,170,128,85,43,0,0,0,0,0,0,0,0},{0,43,85,128,170,213,255,128,0,0,0,0,0,0,0,0}};
    assert(fvm_gradient_init(&g)==FVM_OK);g.type=FVM_GRADIENT_RADIAL;g.matrix.xx=g.matrix.yy=4;g.matrix.x0=4.5;g.matrix.y0=8.5;stop(stops,0,255,0,0,255);stop(stops+1,1,0,0,255,255);g.stops=stops;g.count=2;surface.pixels=pixels;surface.width=surface.height=16;surface.stride=64;
    for(sign=0;sign<2;++sign){memset(pixels,0,sizeof(pixels));result=fvm_path_draw_gradient_focal(path,&surface,&g,0,0,0,sign?0.5:-0.5);assert(result==FVM_OK || result==FVM_UNSUPPORTED);if(result==FVM_UNSUPPORTED)continue;for(x=0;x<16;++x)assert(near(pixels[4*(8*16+x)],red[sign][x]));}
    for(mode=0;mode<3;++mode)for(interpolation=0;interpolation<2;++interpolation)for(stroke=0;stroke<2;++stroke){memset(pixels,0,sizeof(pixels));result=stroke?fvm_path_stroke_gradient_interpolated(path,&surface,&g,4,mode,interpolation):fvm_path_draw_gradient_interpolated(path,&surface,&g,0,mode,interpolation);if(result==FVM_UNSUPPORTED)continue;assert(result==FVM_OK);memcpy(legacy,pixels,sizeof(pixels));memset(pixels,0,sizeof(pixels));assert((stroke?fvm_path_stroke_gradient_focal(path,&surface,&g,4,mode,interpolation,0):fvm_path_draw_gradient_focal(path,&surface,&g,0,mode,interpolation,0))==FVM_OK && !memcmp(legacy,pixels,sizeof(pixels)));}
    memset(pixels,37,sizeof(pixels));memcpy(saved,pixels,sizeof(pixels));assert(fvm_path_draw_gradient_focal(path,&surface,&g,0,0,0,1)==FVM_INVALID_ARGUMENT && !memcmp(pixels,saved,sizeof(pixels)));assert(fvm_path_stroke_gradient_focal(path,&surface,&g,4,0,0,-1)==FVM_INVALID_ARGUMENT);g.type=FVM_GRADIENT_LINEAR;assert(fvm_path_draw_gradient_focal(path,&surface,&g,0,0,0,0.5)==FVM_INVALID_ARGUMENT && !memcmp(pixels,saved,sizeof(pixels)));fvm_path_destroy(path);return 0;
}
