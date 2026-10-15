/* C89 binary32 scene point cases distinguish linear rounding from raw SWF fixed arithmetic. */
#include "flashvm/matrix.h"
#include <assert.h>
#include <limits.h>
int main(void)
{
 FVM_TwipMatrix inverse;FVM_TwipMatrix m={0.5f,0,0,1,0,0};long x,y;unsigned i;
 const long input[]={1,3,5,-1,-3,-5},expected[]={0,2,2,0,-2,-2};
 for(i=0;i<6;++i)assert(fvm_matrix_point_twips(&m,input[i],0,&x,&y)==FVM_OK && x==expected[i] && y==0);
 m.c=0.5f;assert(fvm_matrix_point_twips(&m,1,1,&x,&y)==FVM_OK && x==1 && y==1);
 m.a=1;m.c=0;assert(fvm_matrix_point_twips(&m,2147483647L,0,&x,&y)==FVM_OK && x==(-2147483647L-1L));
 assert(fvm_matrix_point_twips(&m,(-2147483647L-1L),0,&x,&y)==FVM_OK && x==(-2147483647L-1L));
 m.a=2;assert(fvm_matrix_point_twips(&m,(-2147483647L-1L),0,&x,&y)==FVM_OK && x==(-2147483647L-1L));
 m.a=1;assert(fvm_matrix_point_twips(&m,16777217L,0,&x,&y)==FVM_OK && x==16777216L);
 m.tx=1;assert(fvm_matrix_point_twips(&m,16777217L,0,&x,&y)==FVM_OK && x==16777217L);
 m.tx=0;m.c=1;assert(fvm_matrix_point_twips(&m,16777216L,1,&x,&y)==FVM_OK && x==16777216L);
 m.c=0;m.tx=2147483647L;assert(fvm_matrix_point_twips(&m,1,0,&x,&y)==FVM_OK && x==(-2147483647L-1L));
 m.tx=3;m.ty=5;assert(fvm_matrix_point_twips(&m,7,9,&m.tx,&m.ty)==FVM_OK && m.tx==10 && m.ty==14);
 m.a=2;m.b=m.c=0;m.d=4;m.tx=10;m.ty=20;assert(fvm_matrix_inverse_twips(&m,&inverse)==FVM_OK && inverse.a==0.5f && inverse.d==0.25f && inverse.tx==-5 && inverse.ty==-5);assert(fvm_matrix_inverse_twips(&m,&m)==FVM_OK && m.a==0.5f && m.tx==-5);
 m.a=m.d=2;m.tx=1;m.ty=3;assert(fvm_matrix_inverse_twips(&m,&inverse)==FVM_OK && inverse.tx==0 && inverse.ty==-2);inverse.tx=77;m.d=0;assert(fvm_matrix_inverse_twips(&m,&inverse)==FVM_INVALID_ARGUMENT && inverse.tx==77);m.a=1;m.d=0.00000001f;assert(fvm_matrix_inverse_twips(&m,&inverse)==FVM_INVALID_ARGUMENT && inverse.tx==77);m.d=0.00000011920928955078125f;assert(fvm_matrix_inverse_twips(&m,&inverse)==FVM_INVALID_ARGUMENT && inverse.tx==77);m.d=0.0000002384185791015625f;assert(fvm_matrix_inverse_twips(&m,&inverse)==FVM_OK);inverse.tx=77;assert(fvm_matrix_inverse_twips(0,&inverse)==FVM_INVALID_ARGUMENT && inverse.tx==77 && fvm_matrix_inverse_twips(&m,0)==FVM_INVALID_ARGUMENT);
 x=77;y=88;assert(fvm_matrix_point_twips(0,0,0,&x,&y)==FVM_INVALID_ARGUMENT && x==77 && y==88);assert(fvm_matrix_point_twips(&m,0,0,&x,&x)==FVM_INVALID_ARGUMENT && x==77);assert(fvm_matrix_point_twips(&m,0,0,0,&y)==FVM_INVALID_ARGUMENT && y==88);
#if LONG_MAX > 2147483647L
 m.tx=LONG_MAX;assert(fvm_matrix_point_twips(&m,0,0,&x,&y)==FVM_INVALID_ARGUMENT && x==77 && y==88);m.tx=0;assert(fvm_matrix_point_twips(&m,LONG_MAX,0,&x,&y)==FVM_INVALID_ARGUMENT && x==77 && y==88);assert(fvm_matrix_point_twips(&m,0,LONG_MIN,&x,&y)==FVM_INVALID_ARGUMENT && x==77 && y==88);
#endif
 return 0;
}
