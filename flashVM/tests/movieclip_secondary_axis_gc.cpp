#include "secondary_axis_fixture.hpp"
int main() { FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK);ss_context(rt,&c);FVM_Object *target=sa_target(c);
assert(fvm_gc_unpin(c,target)==FVM_OK);for(unsigned n=0;n<48;++n) {target=sa_target(c);int e=n%2?-600:10;FVM_Object *clip=sa_clip(c,(n/2)%3,n%2,e,n/12);assert(fvm_gc_collect(c,0)==FVM_OK);sa_expected(sa_bounds(c,clip,target,e),(n/2)%3);assert(fvm_gc_unpin(c,clip)==FVM_OK&&fvm_gc_unpin(c,target)==FVM_OK&&fvm_gc_collect(c,0)==FVM_OK);assert(!c->gc_native_depth&&!c->gc_frames);}
fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;} 
