/* Every string/character allocation boundary preserves result and active roots on failure. */
#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"
typedef FVM_Result (*Operation)(FVM_Context *,const FVM_Value *,FVM_Value *);
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);Operation ops[]={fvm_exec_string_length,fvm_exec_char_code,fvm_exec_char_from_code};
 for(unsigned i=0;i<3;++i){FVM_Value in,out;if(i==2)fvm_value_number(&in,65);else fvm_value_string_bytes(&in,"a\0b",3);bool success=false;unsigned failures=0;for(long boundary=0;boundary<1000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;fvm_value_number(&out,99);allowance=boundary;FVM_Result r=ops[i](c,&in,&out);allowance=-1;assert(c->gc_frames==roots);if(r==FVM_OK){assert(i==2?(out.type==FVM_VALUE_STRING && fvm_value_string_size(&out)==1):(out.type==FVM_VALUE_NUMBER && out.number==(i?97:3)));success=true;}else{assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==99);++failures;}assert(fvm_gc_collect(c,0)==FVM_OK);}assert(success && failures);}
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
