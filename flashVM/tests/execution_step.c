/* Direct C89 stepping preserves floating arithmetic, aliases and invalid-output atomicity. */
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include <assert.h>
#include <float.h>
int main(void)
{
    FVM_Config cfg; FVM_Runtime *rt; FVM_Context *c;
    FVM_Value v, out; unsigned version;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(version=4;version<=8;++version)
    {
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK);
        fvm_value_number(&v,2147483647.0);
        assert(fvm_exec_step(c,0,&v,&out)==FVM_OK && out.number==2147483648.0);
        assert(fvm_exec_step(c,1,&out,&out)==FVM_OK && out.number==2147483647.0);
        fvm_value_number(&v,-0.5);
        assert(fvm_exec_step(c,0,&v,&v)==FVM_OK && v.number==0.5);
        assert(fvm_value_string(&v,"17")==FVM_OK);
        assert(fvm_exec_step(c,1,&v,&out)==FVM_OK && out.number==16);
        assert(fvm_value_string(&v,"invalid-number")==FVM_OK);
        assert(fvm_exec_step(c,0,&v,&out)==FVM_OK);
        if(version<5) assert(out.number==1);
        else assert(out.number!=out.number);
        fvm_value_number(&v,DBL_MAX);
        assert(fvm_exec_step(c,0,&v,&v)==FVM_OK && v.number==DBL_MAX);
        fvm_value_number(&out,99);
        assert(fvm_exec_step(0,0,&v,&out)==FVM_INVALID_ARGUMENT && out.number==99);
        assert(fvm_exec_step(c,0,0,&out)==FVM_INVALID_ARGUMENT && out.number==99);
        assert(fvm_exec_step(c,0,&v,0)==FVM_INVALID_ARGUMENT);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt); return 0;
}
