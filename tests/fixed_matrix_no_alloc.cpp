/* Fixed matrix services need neither a runtime/context nor any successful heap allocation. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/fixed.h"
int main()
{
 FVM_FixedMatrix m={65536,0,0,65536,3,5},result;long x=0,y=0;allowance=0;
 assert(fvm_fixed_matrix_point(&m,7,9,&x,&y)==FVM_OK && x==10 && y==14);
 assert(fvm_fixed_matrix_multiply(&m,&m,&result)==FVM_OK && result.tx==6 && result.ty==10);
 assert(allowance==0);allowance=-1;return 0;
}
