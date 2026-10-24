#ifndef FVM_PUBLIC_ROUND_SCALE_FIXTURE_HPP
#define FVM_PUBLIC_ROUND_SCALE_FIXTURE_HPP
#include "path_style_hit_fixture.hpp"
#include <cmath>
static void pr_hit(FVM_Path *p,double x,double y,double width,int expected) {
 int out=-1;assert(fvm_path_hit_test(p,x,y,width,0,&out)==FVM_OK);assert(out==expected);
}
static void pr_check(int exponent,unsigned axis=0,bool reflect=false,bool zero=false) {
 double s=std::ldexp(reflect?-1.0:1.0,exponent),w=std::fabs(s)*2;
 double ux=axis==0?1:axis==1?0:.6,uy=axis==0?0:axis==1?1:.8;
 FVM_Path *p=0;assert(fvm_path_create(&p)==FVM_OK);assert(hit_add(p,FVM_PATH_MOVE,0,0)==FVM_OK);
 assert(hit_add(p,FVM_PATH_LINE,zero?0:10*ux*s,zero?0:10*uy*s)==FVM_OK);
 double along=zero?0:5;
 pr_hit(p,(along*ux-.5*uy)*s,(along*uy+.5*ux)*s,w,1);
 pr_hit(p,(along*ux-2*uy)*s,(along*uy+2*ux)*s,w,0);
 pr_hit(p,-.5*ux*s,-.5*uy*s,w,1);
 pr_hit(p,-2*ux*s,-2*uy*s,w,0);
 pr_hit(p,(zero?.5:10.5)*ux*s,(zero?.5:10.5)*uy*s,w,1);
 pr_hit(p,(zero?2:12)*ux*s,(zero?2:12)*uy*s,w,0);
 fvm_path_destroy(p);
}
static void pr_edit(int exponent,bool append) {
 FVM_Path *p=0,*q=0;double s=std::ldexp(1.0,exponent);assert(fvm_path_create(&p)==FVM_OK);
 assert(hit_add(p,FVM_PATH_MOVE,0,0)==FVM_OK);assert(hit_add(p,FVM_PATH_LINE,10,0)==FVM_OK);
 FVM_Affine m={s,0,0,s,0,0};if(append){assert(fvm_path_create(&q)==FVM_OK);assert(fvm_path_append(q,p,&m)==FVM_OK);fvm_path_destroy(p);p=q;}else assert(fvm_path_transform(p,&m)==FVM_OK);
 pr_hit(p,5*s,2*s,2*s,0);pr_hit(p,5*s,.5*s,2*s,1);fvm_path_destroy(p);
}
#endif
