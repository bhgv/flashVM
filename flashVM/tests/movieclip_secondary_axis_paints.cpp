#include "secondary_axis_fixture.hpp"
int main() { FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK);ss_context(rt,&c);FVM_Object *target=sa_target(c);
for(unsigned kind=0;kind<4;++kind)for(int e=-600;e<=10;e+=610)for(unsigned cap=0;cap<3;++cap)for(unsigned join=0;join<2;++join) { FVM_Object *clip=sa_clip(c,cap,join,e,kind);sa_expected(sa_bounds(c,clip,target,e),cap); }
fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;} 
