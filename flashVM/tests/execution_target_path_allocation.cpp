/* Path copying, nullish bare construction and primitive boxing preserve aliases on failure. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *clip=0;
    unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
    assert(fvm_movieclip_create(c,"root",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);
    for(unsigned mode=0;mode<3;++mode)
    {
        bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<2000 && !success;++boundary)
        {
            FVM_Value value;fvm_value_undefined(&value);
            if(!mode){value.type=FVM_VALUE_OBJECT;value.object=clip;}
            if(mode==2)fvm_value_number(&value,17);
            FVM_GCRootFrame *roots=c->gc_frames;
            allowance=boundary;FVM_Result r=fvm_exec_target_path(c,&value,&value);allowance=-1;
            assert(c->gc_frames==roots);
            if(r==FVM_OK){assert(value.type==(mode?FVM_VALUE_UNDEFINED:FVM_VALUE_STRING));success=true;}
            else{assert(r==FVM_OUT_OF_MEMORY);if(!mode)assert(value.type==FVM_VALUE_OBJECT && value.object==clip);if(mode==1)assert(value.type==FVM_VALUE_UNDEFINED);if(mode==2)assert(value.type==FVM_VALUE_NUMBER && value.number==17);++failures;}
            assert(fvm_gc_collect(c,0)==FVM_OK);
        }
        assert(success && failures);
    }
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
