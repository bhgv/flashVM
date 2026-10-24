#ifndef FVM_TRANSLATED_CURVE_FIXTURE_HPP
#define FVM_TRANSLATED_CURVE_FIXTURE_HPP
#include "cubic_scale_fixture.hpp"
/* Independent local-coordinate oracle avoids cancellation in absolute controls. */
static void tc_check(unsigned kind,int exponent,unsigned axis,bool reflect,bool stroke,bool transform) {
 const double table[][4]={{14,53,1,23},{0,60,-60,0},{0,0,60,0},{0,60,0,0},{0,8,16,24},{0,8,0,8},{0,60,0,0},{-40,60,-60,40}};
 double v[4],lo,hi;for(unsigned i=0;i<4;++i)v[i]=(reflect?-1:1)*table[kind][i];
 cb_axis_oracle(v,kind!=6,lo,hi);
 double origin=std::ldexp(reflect?-1.0:1.0,exponent),unit=std::ldexp(1.0,exponent-52);
 double x[4],y[4];for(unsigned i=0;i<4;++i){x[i]=axis?origin+v[i]*unit:0;y[i]=axis==1?0:origin+v[i]*unit;}
 FVM_Path *p=0;assert(fvm_path_create(&p)==FVM_OK);
 if(transform){for(unsigned i=0;i<4;++i){x[i]=axis?v[i]:0;y[i]=axis==1?0:v[i];}}
 assert(hit_add(p,FVM_PATH_MOVE,x[0],y[0])==FVM_OK);
 assert((kind==6?hit_add(p,FVM_PATH_QUADRATIC,x[1],y[1],x[2],y[2]):hit_add(p,FVM_PATH_CUBIC,x[1],y[1],x[2],y[2],x[3],y[3]))==FVM_OK);
 if(transform){FVM_Affine m={unit,0,0,unit,axis?origin:0,axis==1?0:origin};assert(fvm_path_transform(p,&m)==FVM_OK);}
 FVM_PathBounds b;if(stroke)b=stroke_bounds(p,0,0,4*unit);else assert(fvm_path_bounds(p,&b)==FVM_OK);
 double radius=stroke?2:0;
 if(axis){assert(std::fabs((b.x_min-origin)/unit-(lo-radius))<=0.51);assert(std::fabs((b.x_max-origin)/unit-(hi+radius))<=0.51);}
 else {assert(b.x_min==-radius*unit);assert(b.x_max==radius*unit);}
 if(axis!=1){assert(std::fabs((b.y_min-origin)/unit-(lo-radius))<=0.51);assert(std::fabs((b.y_max-origin)/unit-(hi+radius))<=0.51);}
 else {assert(b.y_min==-radius*unit);assert(b.y_max==radius*unit);}
 fvm_path_destroy(p);
}
#endif
