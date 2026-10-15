/* Every frame-preparation allocation failure retains original aliases and caller roots. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
#include "flashvm/execution.h"
#include <cstring>
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    for(unsigned mode=0;mode<3;++mode)
    {
        bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<2000 && !success;++boundary)
        {
            FVM_Value value;if(mode)assert(fvm_value_string(&value,mode==1?"002":"second")==FVM_OK);else fvm_value_number(&value,2);
            FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;FVM_Result r=fvm_exec_frame_request(c,&value,1,3,&value);allowance=-1;assert(c->gc_frames==roots);
            if(r==FVM_OK){if(mode==2)assert(value.type==FVM_VALUE_STRING && std::strcmp(value.string,"second")==0);else assert(value.type==FVM_VALUE_NUMBER && value.number==5);success=true;}
            else {assert(r==FVM_OUT_OF_MEMORY);if(mode)assert(value.type==FVM_VALUE_STRING && std::strcmp(value.string,mode==1?"002":"second")==0);else assert(value.type==FVM_VALUE_NUMBER && value.number==2);++failures;}
            assert(fvm_gc_collect(c,0)==FVM_OK);
        }
        assert(success && failures);
    }
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
