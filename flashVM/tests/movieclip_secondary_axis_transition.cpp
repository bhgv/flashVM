#include "secondary_axis_fixture.hpp"
int main() { FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK);ss_context(rt,&c);FVM_Object *target=sa_target(c);
const int exponents[]={-509,-510,-511,-512,10,17,18,510};for(unsigned e=0;e<sizeof(exponents)/sizeof(exponents[0]);++e) for(unsigned cap=0;cap<3;++cap)for(unsigned join=0;join<2;++join) { int exponent=exponents[e];FVM_Object *clip=sa_clip(c,cap,join,exponent);if(exponent>17)sa_rejected(c,clip,target);else sa_expected(sa_bounds(c,clip,target,exponent),cap); }
fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;} 
