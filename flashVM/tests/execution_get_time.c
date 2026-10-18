/* Deterministic link-time clock proves context origins, unsigned wrap and failure atomicity. */
#include "flashvm/execution.h"
#include "flashvm/avm1.h"
#include "../ports/monotonic_port.h"
#include <assert.h>
#include <math.h>
static double sample;
static int available=1;
static unsigned reads;
int fvm_port_monotonic_ms(double *output)
{
    ++reads;if(!available || !output)return 0;*output=sample;return 1;
}
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;unsigned version;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(version=4;version<=8;++version)
    {
        FVM_Context *c;FVM_Value out;unsigned before;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        const unsigned char actions[]={0x96,6,0,0,'t','i','c','k',0,0x34,0x1d,0};
        swf[3]=(unsigned char)version;sample=1000.25;
        assert(fvm_context_create(rt,&c)==FVM_OK);
        sample=1042.99;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_avm1_execute(c,actions,sizeof(actions))==FVM_OK);
        assert(fvm_get_global(c,"tick",&out)==FVM_OK && out.type==FVM_VALUE_NUMBER && out.number==42);
        sample=1234.99;assert(fvm_exec_get_time(c,&out)==FVM_OK && out.number==234);
        sample=1100;assert(fvm_exec_get_time(c,&out)==FVM_OK && out.number==234);
        sample=1000.25+2147483648.99;
        assert(fvm_exec_get_time(c,&out)==FVM_OK && out.number==2147483648.0);
        sample=1000.25+4294967296.75;
        assert(fvm_exec_get_time(c,&out)==FVM_OK && out.number==0);
        sample=1000.25+4294967297.25;
        assert(fvm_exec_get_time(c,&out)==FVM_OK && out.number==1);
        available=0;fvm_value_number(&out,99);
        assert(fvm_exec_get_time(c,&out)==FVM_ERROR && out.number==99);
        available=1;sample=HUGE_VAL;
        assert(fvm_exec_get_time(c,&out)==FVM_ERROR && out.number==99);
        sample=-1;assert(fvm_exec_get_time(c,&out)==FVM_ERROR && out.number==99);
        sample=1000;assert(fvm_exec_get_time(c,&out)==FVM_OK && out.number==1);
        before=reads;
        assert(fvm_exec_get_time(0,&out)==FVM_INVALID_ARGUMENT);
        assert(fvm_exec_get_time(c,0)==FVM_INVALID_ARGUMENT && reads==before);
        fvm_context_destroy(c);
    }
    {
        FVM_Context *a,*b;FVM_Value out;
        available=0;assert(fvm_context_create(rt,&a)==FVM_OK);fvm_value_number(&out,99);
        assert(fvm_exec_get_time(a,&out)==FVM_ERROR && out.number==99);
        available=1;sample=2000;assert(fvm_exec_get_time(a,&out)==FVM_OK && out.number==0);
        sample=2005.99;assert(fvm_exec_get_time(a,&out)==FVM_OK && out.number==5);
        sample=2100;assert(fvm_context_create(rt,&b)==FVM_OK);
        sample=2200;
        assert(fvm_exec_get_time(a,&out)==FVM_OK && out.number==200);
        assert(fvm_exec_get_time(b,&out)==FVM_OK && out.number==100);
        fvm_context_destroy(b);fvm_context_destroy(a);
    }
    fvm_runtime_destroy(rt);return 0;
}
