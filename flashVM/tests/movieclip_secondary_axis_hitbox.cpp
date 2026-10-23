#include "secondary_axis_fixture.hpp"
int main() { FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK);ss_context(rt,&c);FVM_Object *target=sa_target(c);
for(unsigned cap=0;cap<3;++cap)for(unsigned join=0;join<2;++join) {FVM_Object *clip=sa_clip(c,cap,join,-600);double s=sa_output(-600);for(unsigned shape=0;shape<2;++shape){FVM_Value a[3],out;fvm_value_number(a,15*s);fvm_value_number(a+1,20*s);fvm_value_boolean(a+2,shape!=0);assert(fvm_object_call(c,clip,"hitTest",3,a,&out)==FVM_OK&&out.boolean);fvm_value_number(a,90*s);fvm_value_number(a+1,90*s);assert(fvm_object_call(c,clip,"hitTest",3,a,&out)==FVM_OK&&!out.boolean);}}
fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;} 
