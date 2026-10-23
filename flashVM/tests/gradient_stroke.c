/* C89 stroke pixels match a separately rendered white stroke over a sampled color ramp. */
#define main gradient_reference_main
#include "gradient.c"
#undef main
#include <limits.h>
#include <stddef.h>
/* The original public layout remains unchanged by the additive stroke entry. */
typedef struct LegacyGradient {FVM_GradientType type;FVM_Affine matrix;const FVM_GradientStop *stops;size_t count;double multiply[4];double add[4];} LegacyGradient;
int main(void)
{
    FVM_Path *path=0;FVM_PathCommand command;FVM_Gradient gradient;FVM_GradientStop stops[2];unsigned mode,type,x,y;unsigned char actual[1024],mask[1024],ramp[1024],saved[1024];FVM_RasterSurface dst,coverage,source;FVM_Path *rect=box();FVM_RasterColor white={255,255,255,255};FVM_Result result;
    assert(sizeof(FVM_Gradient)==sizeof(LegacyGradient) && offsetof(FVM_Gradient,add)==offsetof(LegacyGradient,add) && offsetof(FVM_Gradient,stops)==offsetof(LegacyGradient,stops));
    dst.pixels=actual;coverage.pixels=mask;source.pixels=ramp;dst.width=coverage.width=source.width=16;dst.height=coverage.height=source.height=16;dst.stride=coverage.stride=source.stride=64;
    assert(fvm_path_create(&path)==FVM_OK);memset(&command,0,sizeof(command));command.verb=FVM_PATH_MOVE;command.x[0]=1;command.y[0]=8;assert(fvm_path_add(path,&command)==FVM_OK);command.verb=FVM_PATH_LINE;command.x[0]=15;assert(fvm_path_add(path,&command)==FVM_OK);
    assert(fvm_gradient_init(&gradient)==FVM_OK);stop(stops,0,255,0,0,255);stop(stops+1,1,0,0,255,255);gradient.stops=stops;gradient.count=2;gradient.matrix.xx=gradient.matrix.yy=2;gradient.matrix.x0=4.5;gradient.matrix.y0=8.5;
    for(type=0;type<2;++type)for(mode=0;mode<3;++mode){gradient.type=(FVM_GradientType)type;memset(actual,0,sizeof(actual));memset(mask,0,sizeof(mask));memset(ramp,0,sizeof(ramp));result=fvm_path_stroke_gradient(path,&dst,&gradient,4,mode);assert(result==FVM_OK || result==FVM_UNSUPPORTED);if(result==FVM_UNSUPPORTED)continue;assert(fvm_path_draw(path,&coverage,white,4,0)==FVM_OK && fvm_path_draw_gradient_spread(rect,&source,&gradient,0,mode)==FVM_OK);for(y=0;y<16;++y)for(x=0;x<16;++x){unsigned at=4*(16*y+x);assert(actual[at+3]==mask[at+3]);if(mask[at+3])assert(!memcmp(actual+at,ramp+at,3));}assert(actual[(8*16+8)*4+3]==255 && actual[(2*16+8)*4+3]==0);}
    memset(actual,37,sizeof(actual));memcpy(saved,actual,sizeof(actual));assert(fvm_path_stroke_gradient(path,&dst,&gradient,0,0)==FVM_INVALID_ARGUMENT && !memcmp(actual,saved,sizeof(actual)));assert(fvm_path_stroke_gradient(path,&dst,&gradient,-1,0)==FVM_INVALID_ARGUMENT);assert(fvm_path_stroke_gradient(path,&dst,&gradient,4097,0)==FVM_INVALID_ARGUMENT);assert(fvm_path_stroke_gradient(path,&dst,&gradient,1,UINT_MAX)==FVM_INVALID_ARGUMENT && !memcmp(actual,saved,sizeof(actual)));assert(fvm_path_stroke_gradient(path,&dst,&gradient,HUGE_VAL,0)==FVM_INVALID_ARGUMENT && !memcmp(actual,saved,sizeof(actual)));assert(fvm_path_stroke_gradient(0,&dst,&gradient,1,0)==FVM_INVALID_ARGUMENT && fvm_path_stroke_gradient(path,&dst,0,1,0)==FVM_INVALID_ARGUMENT && !memcmp(actual,saved,sizeof(actual)));fvm_path_destroy(rect);fvm_path_destroy(path);return 0;
}
