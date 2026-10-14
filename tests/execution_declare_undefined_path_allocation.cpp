/* Qualified undefined declarations retain roots across lookup and setter allocation failures. */
#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"
static unsigned writes;
static FVM_Result get(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *){fvm_value_undefined(out);return FVM_OK;}
static FVM_Result put(FVM_Context *,FVM_Object *,unsigned n,const FVM_Value *a,FVM_Value *out,void *){assert(n==1 && a[0].type==FVM_VALUE_UNDEFINED);++writes;fvm_value_undefined(out);return FVM_OK;}
int main(){FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *holder=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_object_create(c,&holder)==FVM_OK);FVM_Value key,value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=holder;assert(fvm_set_global(c,"holder",&value)==FVM_OK && fvm_object_define_accessor_rooted(c,holder,"x",get,put,0,0)==FVM_OK);fvm_value_string(&key,"holder:x");bool success=false;unsigned failures=0;for(long boundary=0;boundary<2000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;unsigned depth=c->string_depth;unsigned before=writes;allowance=boundary;FVM_Result r=fvm_exec_variable_declare(c,0,0,0,0,FVM_EXEC_DECLARE_UNINITIALIZED,&key,0);allowance=-1;assert(c->gc_frames==roots && c->string_depth==depth);if(r==FVM_OK){assert(writes==before+1);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && writes==before);++failures;}assert(fvm_gc_collect(c,0)==FVM_OK);}assert(success && failures);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;}
