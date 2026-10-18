/* With creation retains the lexical parent and preserves output at every allocation boundary. */
#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"
int main(){FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);for(unsigned missing=0;missing<2;++missing){FVM_Context *c=0;FVM_Object *parent=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_object_create(c,&parent)==FVM_OK && fvm_gc_pin(c,parent)==FVM_OK);FVM_Value input,value;fvm_value_number(&input,42);fvm_value_undefined(&value);if(missing)assert(fvm_set_global(c,"Number",&value)==FVM_OK);bool success=false;unsigned failures=0;for(long boundary=0;boundary<2000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;FVM_Object *scope=parent;allowance=boundary;FVM_Result r=fvm_exec_with_scope(c,parent,&input,&scope);allowance=-1;assert(c->gc_frames==roots);if(r==FVM_OK){assert(scope!=parent);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && scope==parent);++failures;}assert(fvm_gc_collect(c,0)==FVM_OK);}assert(success && failures);fvm_context_destroy(c);}fvm_runtime_destroy(rt);return 0;}
