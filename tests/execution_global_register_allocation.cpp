/* Allocation failure cannot replace a retained register or publish a partial result. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    bool failed=false,succeeded=false;
    for(long budget=0;budget<80;++budget) {
        allowance=-1;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        FVM_Value old_value,input,out;fvm_value_number(&old_value,12);assert(fvm_exec_global_register(c,1,0,&old_value,&out)==FVM_OK);
        fvm_value_string(&input,"a retained register string");fvm_value_number(&out,73);
        allowance=budget;FVM_Result status=fvm_exec_global_register(c,1,0,&input,&out);allowance=-1;
        FVM_Value stored;assert(fvm_exec_global_register(c,0,0,0,&stored)==FVM_OK);
        if(status==FVM_OUT_OF_MEMORY) { failed=true;assert(out.type==FVM_VALUE_NUMBER && out.number==73 && stored.type==FVM_VALUE_NUMBER && stored.number==12); }
        else { assert(status==FVM_OK && stored.type==FVM_VALUE_STRING);succeeded=true; }
        assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);if(succeeded)break;
    }
    assert(failed && succeeded);fvm_runtime_destroy(rt);
}
