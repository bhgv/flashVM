/* C89 inverse projection: dense matrix, displacements, aliasing and atomic errors. */
#include "flashvm/affine.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
static void near(double actual,double expected){assert(fabs(actual-expected)<1e-12);}
int main(void)
{
 FVM_Affine m={2,1,0.5,3,5,7}, singular={1,2,2,4,0,0};
 double x=73,y=74,nan=strtod("NaN",0);
 assert(fvm_affine_inverse_point(&m,FVM_AFFINE_POINT,21,-21,&x,&y)==FVM_OK);near(x,124.0/11.0);near(y,-144.0/11.0);
 assert(fvm_affine_inverse_point(&m,FVM_AFFINE_VECTOR,16,-28,&x,&y)==FVM_OK);near(x,124.0/11.0);near(y,-144.0/11.0);
 assert(fvm_affine_inverse_point(&m,FVM_AFFINE_POINT,21,-21,&m.xx,&m.yy)==FVM_OK);near(m.xx,124.0/11.0);near(m.yy,-144.0/11.0);
 x=73;y=74;
 assert(fvm_affine_inverse_point(&singular,FVM_AFFINE_POINT,1,2,&x,&y)==FVM_INVALID_ARGUMENT && x==73 && y==74);
 assert(fvm_affine_inverse_point(0,FVM_AFFINE_POINT,1,2,&x,&y)==FVM_INVALID_ARGUMENT && x==73 && y==74);
 assert(fvm_affine_inverse_point(&m,2,1,2,&x,&y)==FVM_INVALID_ARGUMENT && x==73 && y==74);
 assert(fvm_affine_inverse_point(&m,FVM_AFFINE_VECTOR,1,2,&x,&x)==FVM_INVALID_ARGUMENT && x==73);
 assert(fvm_affine_inverse_point(&m,FVM_AFFINE_VECTOR,1,2,0,&y)==FVM_INVALID_ARGUMENT && y==74);
 m.xx=1;m.xy=0;m.yx=0;m.yy=1;m.x0=nan;m.y0=HUGE_VAL;
 assert(fvm_affine_inverse_point(&m,FVM_AFFINE_VECTOR,1,2,&x,&y)==FVM_OK && x==1 && y==2);
 x=73;y=74;
 assert(fvm_affine_inverse_point(&m,FVM_AFFINE_POINT,1,2,&x,&y)==FVM_INVALID_ARGUMENT && x==73 && y==74);
 m.x0=0;m.y0=0;
 assert(fvm_affine_inverse_point(&m,FVM_AFFINE_POINT,nan,2,&x,&y)==FVM_INVALID_ARGUMENT && x==73 && y==74);
 m.xx=DBL_MAX;m.yy=2;
 assert(fvm_affine_inverse_point(&m,FVM_AFFINE_VECTOR,1,2,&x,&y)==FVM_INVALID_ARGUMENT && x==73 && y==74);
 m.xx=0.25;m.yy=1;
 assert(fvm_affine_inverse_point(&m,FVM_AFFINE_VECTOR,DBL_MAX,2,&x,&y)==FVM_INVALID_ARGUMENT && x==73 && y==74);
 return 0;
}
