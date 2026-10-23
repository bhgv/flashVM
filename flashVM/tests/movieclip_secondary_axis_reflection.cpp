#include "secondary_axis_fixture.hpp"
int main() { FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK);ss_context(rt,&c);FVM_Object *target=sa_target(c);
for(int exponent=-600;exponent<=10;exponent+=610)for(unsigned cap=0;cap<3;++cap)for(unsigned join=0;join<2;++join) { FVM_Object *clip=sa_clip(c,cap,join,exponent,0,true);sa_expected(sa_bounds(c,clip,target,exponent),cap,true); }
fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;} 
