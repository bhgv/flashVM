/* Actual SetVariable converts numeric names in loaded SWF4 through SWF8. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
int main(){FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);Bytes code;integer(code,17);integer(code,42);code.push_back(0x1d);text(code,"17");code.push_back(0x1c);FVM_Value out=run(c,code);assert(out.type==FVM_VALUE_NUMBER && out.number==42);fvm_context_destroy(c);}fvm_runtime_destroy(rt);return 0;}
