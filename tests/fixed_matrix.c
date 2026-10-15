/* C89 context-free matrix APIs checked against independent arbitrary-precision fixtures. */
#include "flashvm/fixed.h"
#include <assert.h>
#include <limits.h>
#include "fixed_matrix_vectors.h"
static void same(const FVM_FixedMatrix *a,const FVM_FixedMatrix *b)
{assert(a->a==b->a && a->b==b->b && a->c==b->c && a->d==b->d && a->tx==b->tx && a->ty==b->ty);}
int main(void)
{
 unsigned i;long x,y;FVM_FixedMatrix a,b,out,identity={65536,0,0,65536,0,0};
 for(i=0;i<sizeof(fixed_products)/sizeof(fixed_products[0]);++i){a=identity;a.a=fixed_products[i].a;x=77;y=88;assert(fvm_fixed_matrix_point(&a,fixed_products[i].b,0,&x,&y)==FVM_OK && x==fixed_products[i].result && y==0);}
 /* Separately shifted half products do not cancel before truncation: floor(.5)+floor(-.5)=-1. */
 a=identity;a.a=32768;a.c=32768;assert(fvm_fixed_matrix_point(&a,1,-1,&x,&y)==FVM_OK && x==-1 && y==-1);
 a=identity;a.tx=2147483647L;a.ty=(-2147483647L-1L);assert(fvm_fixed_matrix_point(&a,1,-1,&x,&y)==FVM_OK && x==(-2147483647L-1L) && y==2147483647L);
 for(i=0;i<sizeof(fixed_matrices)/sizeof(fixed_matrices[0]);++i){const struct FixedMatrixCase *v=fixed_matrices+i;assert(fvm_fixed_matrix_multiply(&v->left,&v->right,&out)==FVM_OK);same(&out,&v->result);a=v->left;b=v->right;assert(fvm_fixed_matrix_multiply(&a,&b,&a)==FVM_OK);same(&a,&v->result);a=v->left;b=v->right;assert(fvm_fixed_matrix_multiply(&a,&b,&b)==FVM_OK);same(&b,&v->result);}
 a=identity;assert(fvm_fixed_matrix_multiply(&a,&a,&a)==FVM_OK);same(&a,&identity);
 a=identity;assert(fvm_fixed_matrix_point(&a,7,9,&a.a,&a.b)==FVM_OK && a.a==7 && a.b==9);
 x=77;y=88;out=identity;assert(fvm_fixed_matrix_point(0,0,0,&x,&y)==FVM_INVALID_ARGUMENT && x==77 && y==88);assert(fvm_fixed_matrix_point(&identity,0,0,&x,&x)==FVM_INVALID_ARGUMENT && x==77);assert(fvm_fixed_matrix_point(&identity,0,0,0,&y)==FVM_INVALID_ARGUMENT && y==88);assert(fvm_fixed_matrix_multiply(0,&identity,&out)==FVM_INVALID_ARGUMENT);same(&out,&identity);assert(fvm_fixed_matrix_multiply(&identity,&identity,0)==FVM_INVALID_ARGUMENT);
#if LONG_MAX > 2147483647L
 {long *field[6];field[0]=&a.a;field[1]=&a.b;field[2]=&a.c;field[3]=&a.d;field[4]=&a.tx;field[5]=&a.ty;for(i=0;i<6;++i){a=identity;*field[i]=LONG_MAX;x=77;y=88;assert(fvm_fixed_matrix_point(&a,0,0,&x,&y)==FVM_INVALID_ARGUMENT && x==77 && y==88);assert(fvm_fixed_matrix_multiply(&a,&identity,&out)==FVM_INVALID_ARGUMENT);same(&out,&identity);assert(fvm_fixed_matrix_multiply(&identity,&a,&out)==FVM_INVALID_ARGUMENT);same(&out,&identity);}assert(fvm_fixed_matrix_point(&identity,LONG_MAX,0,&x,&y)==FVM_INVALID_ARGUMENT && x==77 && y==88);assert(fvm_fixed_matrix_point(&identity,0,LONG_MIN,&x,&y)==FVM_INVALID_ARGUMENT && x==77 && y==88);}
#endif
 return 0;
}
