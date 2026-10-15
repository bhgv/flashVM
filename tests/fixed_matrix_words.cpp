/* Exercise 32-bit uint and native ulong limb arithmetic against independent big-integer vectors. */
#include "../src/core/fixed32.hpp"
#include "fixed_matrix_vectors.h"
#include <cassert>
#include <climits>
int main()
{
 for(unsigned i=0;i<sizeof(fixed_products)/sizeof(fixed_products[0]);++i){const FixedProductCase &v=fixed_products[i];assert(fvm_fixed_detail::signed_word(fvm_fixed_detail::product_word<unsigned long>(v.a,v.b))==v.result);
#if UINT_MAX >= 0xffffffffUL
 assert(fvm_fixed_detail::signed_word(fvm_fixed_detail::product_word<unsigned int>(v.a,v.b))==v.result);
#endif
 }
 return 0;
}
