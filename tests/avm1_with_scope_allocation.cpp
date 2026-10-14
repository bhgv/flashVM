/* Scope creation preserves caller outputs and root chains after allocation failure. */
#define main existing_allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    bool failed=false,succeeded=false;
    for(long budget=0;budget<100;++budget) {
        FVM_Context *c=0;allowance=-1;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
        FVM_Value input;fvm_value_string(&input,"scope value");FVM_Object *sentinel=c->global,*scope=sentinel;
        allowance=budget;FVM_Result status=fvm_exec_with_scope(c,0,&input,&scope);allowance=-1;
        assert(!c->gc_frames);
        if(status==FVM_OUT_OF_MEMORY) { failed=true;assert(scope==sentinel); }
        else { assert(status==FVM_OK && scope!=sentinel);succeeded=true; }
        assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
        if(succeeded)break;
    }
    assert(failed && succeeded);fvm_runtime_destroy(rt);
}
