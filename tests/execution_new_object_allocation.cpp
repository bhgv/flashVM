/* Named constructor allocation failure preserves result and invocation/root state. */
#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "../src/core/runtime.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);const unsigned char code[]={0};FVM_Object *fn=0;assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK);FVM_Value value,key,arg,out;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=fn;assert(fvm_set_global(c,"Ctor",&value)==FVM_OK);fvm_value_string(&key,"Ctor");fvm_value_string(&arg,"retained");bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<3000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;FVM_Object *old_fn=c->calling_function,*old_owner=c->calling_owner,*old_ctor=c->calling_constructor;unsigned native=c->gc_native_depth,script=c->script_depth;fvm_value_number(&out,99);allowance=boundary;FVM_Result r=fvm_exec_new_object(c,0,0,0,0,&key,1,&arg,&out);allowance=-1;assert(c->gc_frames==roots && c->calling_function==old_fn && c->calling_owner==old_owner && c->calling_constructor==old_ctor && c->gc_native_depth==native && c->script_depth==script);if(r==FVM_OK){assert(out.type==FVM_VALUE_OBJECT);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==99);++failures;}assert(fvm_gc_collect(c,0)==FVM_OK);}
 assert(success && failures);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
