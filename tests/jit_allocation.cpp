/* Compile/invoke allocation failures never unwind through executable pages or leak active roots. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/jit.h"
int main()
{
 if(!fvm_jit_available())return 0;const unsigned char bytes[]={0x96,5,0,7,20,0,0,0,0x96,5,0,7,22,0,0,0,0x0a,0x3e,0};
 FVM_JitCode *code=0;bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<2000 && !success;++boundary){allowance=boundary;FVM_Result r=fvm_jit_compile(bytes,sizeof(bytes),&code);allowance=-1;if(r==FVM_OK)success=true;else{assert(r==FVM_OUT_OF_MEMORY && !code);++failures;}}
 assert(success && failures && fvm_jit_destroy(code)==FVM_OK);
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *fn=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_function_create_avm1(c,bytes,sizeof(bytes),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(bytes,sizeof(bytes),&code)==FVM_OK && fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);
 success=false;failures=0;
 for(long boundary=0;boundary<2000 && !success;++boundary){FVM_Value out;FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;FVM_Result r=fvm_function_invoke(c,fn,0,0,0,&out);allowance=-1;assert(c->gc_frames==roots && !c->gc_native_depth && !c->script_depth && !c->avm1_execution_depth);if(r==FVM_OK){assert(out.number==42);success=true;}else{assert(r==FVM_OUT_OF_MEMORY);++failures;}assert(fvm_gc_collect(c,0)==FVM_OK);}
 assert(success && failures && fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
