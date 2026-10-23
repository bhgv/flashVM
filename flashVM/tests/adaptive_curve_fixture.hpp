#ifndef FVM_ADAPTIVE_CURVE_FIXTURE_HPP
#define FVM_ADAPTIVE_CURVE_FIXTURE_HPP
#include "linear_curve_fixture.hpp"
static FVM_Path *dc_path(unsigned degree=2,bool reverse=false,int exponent=-8,double origin=0)
{ FVM_Path *p=lc_path(degree,reverse); double scale=std::ldexp(1.0,exponent); FVM_Affine m={scale,0,0,scale,origin,origin}; assert(fvm_path_transform(p,&m)==FVM_OK); return p; }
static void dc_expected(const FVM_PathBounds &b,unsigned cap,double width=4.0/256,int exponent=-8,double origin=0)
{ double scale=std::ldexp(1.0,exponent); lc_expected(b,cap,width,origin+2*scale,origin+4*scale,origin+14*scale,origin+10*scale); }
static FVM_Object *dc_clip(FVM_Context *c,unsigned cap,unsigned join,double width=4.0/256,unsigned kind=0)
{ FVM_Object *clip=lc_clip(c,cap,join,width,kind),*commands=rc_part(clip)->properties["commands"].object; for(unsigned i=0;i<2;++i) { FVM_Object *cmd=commands->indexed[i].object; rc_number(cmd,"x0",cmd->properties["x0"].number/256); rc_number(cmd,"y0",cmd->properties["y0"].number/256); if(i) { rc_number(cmd,"x1",cmd->properties["x1"].number/256); rc_number(cmd,"y1",cmd->properties["y1"].number/256); } } return clip; }
#endif
