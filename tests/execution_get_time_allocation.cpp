/* Reading elapsed time allocates no VM memory and changes no GC roots or objects. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Value value;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    FVM_GCRootFrame *roots=c->gc_frames;unsigned long before=0,after=0;
    assert(fvm_gc_object_count(c,&before)==FVM_OK);
    allowance=0;
    for(unsigned i=0;i<128;++i)assert(fvm_exec_get_time(c,&value)==FVM_OK && value.type==FVM_VALUE_NUMBER);
    allowance=-1;
    assert(c->gc_frames==roots && fvm_gc_object_count(c,&after)==FVM_OK && before==after);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
