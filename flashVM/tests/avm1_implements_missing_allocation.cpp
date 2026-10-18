/* Empty/single-value ImplementsOp restores execution/root scopes at every allocation failure. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
#include "flashvm/avm1.h"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version)for(unsigned one=0;one<2;++one)
    {
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        const unsigned char none[]={0x2c,0},single[]={0x96,5,0,7,7,0,0,0,0x2c,0};const unsigned char *code=one?single:none;size_t size=one?sizeof(single):sizeof(none);bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<2000 && !success;++boundary)
        {
            FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;FVM_Result r=fvm_avm1_execute(c,code,size);allowance=-1;
            assert(c->gc_frames==roots && !c->avm1_execution_depth && !c->script_depth);
            if(r==FVM_OK)success=true;else{assert(r==FVM_OUT_OF_MEMORY);++failures;}
            assert(fvm_gc_collect(c,0)==FVM_OK);
        }
        assert(success && failures);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
