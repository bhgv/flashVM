/* Fixed clock compares GetTime, direct globals and aliases through real native JIT entries. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "../ports/monotonic_port.h"
static double sample;
static int available=1;
extern "C" int fvm_port_monotonic_ms(double *out)
{ if(!available)return 0;*out=sample;return 1; }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version)
    for(class_mode=0;class_mode<(fvm_jit_available()?3U:1U);++class_mode) {
        sample=1000;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        FVM_Value fn;assert(fvm_get_global(c,"getTimer",&fn)==FVM_OK && fvm_set_global(c,"clockAlias",&fn)==FVM_OK);
        Program action;action.op(0x34);action.finish();
        Program direct;direct.number(0);direct.text("getTimer");direct.op(0x3d);direct.finish();
        Program alias;alias.text("ignored");alias.number(1);alias.text("clockAlias");alias.op(0x3d);alias.finish();
        const double elapsed[]={42.99,2147483648.75,4294967297.5};const double expected[]={42,2147483648.0,1};
        for(unsigned i=0;i<3;++i) {
            sample=1000+elapsed[i];FVM_Value v=class_run(c,action);assert(v.type==FVM_VALUE_NUMBER && v.number==expected[i]);
            v=class_run(c,direct);assert(v.type==FVM_VALUE_NUMBER && v.number==expected[i]);
            v=class_run(c,alias);assert(v.type==FVM_VALUE_NUMBER && v.number==expected[i]);
        }
        available=0;class_run(c,direct,FVM_ERROR);available=1;
        assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
