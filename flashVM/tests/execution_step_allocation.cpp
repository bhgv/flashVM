/* Step failures preserve aliased input and restore the caller's root chain. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
    bool success=false;unsigned failures=0;
    for(long boundary=0;boundary<128 && !success;++boundary)
    {
        FVM_Value value;assert(fvm_value_string(&value,"17")==FVM_OK);
        FVM_GCRootFrame *roots=c->gc_frames;
        allowance=boundary;FVM_Result r=fvm_exec_step(c,0,&value,&value);allowance=-1;
        assert(c->gc_frames==roots);
        if(r==FVM_OK){assert(value.type==FVM_VALUE_NUMBER && value.number==18);success=true;}
        else{assert(r==FVM_OUT_OF_MEMORY && value.type==FVM_VALUE_STRING);++failures;}
    }
    assert(success && failures);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
