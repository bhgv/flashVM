/* GetVariable conversion/lookup allocation failures preserve roots and output. */
#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);FVM_Value value,key,out;fvm_value_number(&value,42);assert(fvm_set_global(c,"17",&value)==FVM_OK);fvm_value_number(&key,17);bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<2000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;unsigned string_depth=c->string_depth;fvm_value_number(&out,99);allowance=boundary;FVM_Result r=fvm_exec_get_variable(c,0,0,0,0,&key,&out);allowance=-1;assert(c->gc_frames==roots && c->string_depth==string_depth);if(r==FVM_OK){assert(out.type==FVM_VALUE_NUMBER && out.number==42);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==99);++failures;}assert(fvm_gc_collect(c,0)==FVM_OK);}assert(success && failures);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
