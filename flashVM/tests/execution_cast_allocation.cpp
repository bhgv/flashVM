/* Cast primitive boxing preserves output and restores roots at allocation failures. */
#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
int main(){FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);FVM_Value input,klass,out;fvm_value_number(&input,42);assert(fvm_get_global(c,"Object",&klass)==FVM_OK);bool success=false;unsigned failures=0;for(long boundary=0;boundary<2000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;fvm_value_number(&out,99);allowance=boundary;FVM_Result r=fvm_exec_cast(c,&input,&klass,&out);allowance=-1;assert(c->gc_frames==roots);if(r==FVM_OK){assert(out.type==FVM_VALUE_NULL);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==99);++failures;}assert(fvm_gc_collect(c,0)==FVM_OK);}assert(success && failures);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;}
