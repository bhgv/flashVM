#ifndef FVM_SECONDARY_AXIS_FIXTURE_HPP
#define FVM_SECONDARY_AXIS_FIXTURE_HPP
#include "linear_curve_fixture.hpp"
static double sa_unit(int exponent) { return exponent>0?std::ldexp(1.0,-exponent):1; }
static double sa_output(int exponent) { return exponent<0?std::ldexp(1.0,exponent):1; }
static FVM_Object *sa_target(FVM_Context *c) { return rb_clip(c,4); }
static FVM_Object *sa_clip(FVM_Context *c,unsigned cap,unsigned join,int exponent,unsigned kind=0,bool reflected=false,double xcolumn=0)
{
 FVM_Object *clip=0;assert(fvm_movieclip_create(c,"axis",1,&clip)==FVM_OK&&fvm_gc_pin(c,clip)==FVM_OK);
 double unit=sa_unit(exponent),scale=std::ldexp(1.0,exponent);ss_style(c,clip,cap,join,4*unit);
 if(kind) { FVM_Value args[8],out;spread_arguments(c,args,1,kind>1);if(kind==3)fvm_value_number(args+7,.5);assert(fvm_object_call(c,clip,"lineGradientStyle",8,args,&out)==FVM_OK); }
 ss_xy(c,clip,"moveTo",0,0);ss_xy(c,clip,"lineTo",0,10*unit);
 FVM_Affine m={xcolumn*scale,0,(reflected?-3:3)*scale,(reflected?-4:4)*scale,0,0};assert(fvm_movieclip_set_matrix(c,clip,&m)==FVM_OK);return clip;
}
static FVM_PathBounds sa_bounds(FVM_Context *c,FVM_Object *clip,FVM_Object *target,int exponent,const char *method="getBounds")
{
 FVM_Value arg,out;ref(&arg,target);assert(fvm_object_call(c,clip,method,1,&arg,&out)==FVM_OK);
 FVM_Object *b=out.object;double s=sa_output(exponent);FVM_PathBounds result={b->properties["xMin"].number/s,b->properties["yMin"].number/s,b->properties["xMax"].number/s,b->properties["yMax"].number/s};return result;
}
static void sa_expected(const FVM_PathBounds &b,unsigned cap,bool reverse=false,double end=10)
{ lc_expected(b,cap,20,reverse?-3*end:0,reverse?-4*end:0,reverse?0:3*end,reverse?0:4*end); }
#endif
