/* Fixed matrix services need neither a runtime/context nor any successful heap allocation. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/fixed.h"
#include "flashvm/matrix.h"
#include <limits>
int main()
{
 FVM_FixedMatrix m={65536,0,0,65536,3,5},result;long x=0,y=0;allowance=0;
 assert(fvm_fixed_matrix_point(&m,7,9,&x,&y)==FVM_OK && x==10 && y==14);
 assert(fvm_fixed_matrix_multiply(&m,&m,&result)==FVM_OK && result.tx==6 && result.ty==10);
 FVM_TwipMatrix scene={0.5f,0,0,0.5f,3,5};assert(fvm_matrix_point_twips(&scene,1,3,&x,&y)==FVM_OK && x==3 && y==7);FVM_TwipMatrix inverse;assert(fvm_matrix_inverse_twips(&scene,&inverse)==FVM_OK && inverse.a==2 && inverse.d==2 && inverse.tx==-6 && inverse.ty==-10);scene.a=std::numeric_limits<float>::quiet_NaN();scene.d=std::numeric_limits<float>::infinity();assert(fvm_matrix_point_twips(&scene,1,1,&x,&y)==FVM_OK && x==3 && y==5);
 assert(fvm_matrix_inverse_twips(&scene,&inverse)==FVM_INVALID_ARGUMENT && inverse.tx==-6);
 assert(allowance==0);allowance=-1;return 0;
}
