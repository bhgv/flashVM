/* C89 entry options select interpreter/JIT, cache supported bodies and retain unsupported fallback. */
#include "flashvm/execution_mode.h"
#include "flashvm/standard.h"
#include <assert.h>
int main(void)
{
 const unsigned char body[]={0x96,3,0,0,'x',0,0x1c,0x96,5,0,7,22,0,0,0,0x0a,0x3e,0};
 /* An unreachable reserved opcode 0x16 retains whole-body interpreter fallback. */
 const unsigned char unsupported[]={0x99,2,0,1,0,0x16,0x96,5,0,7,42,0,0,0,0x3e,0};const char *params[]={"x"};
 FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *fn,*fallback;FVM_Value arg,out;FVM_ExecutionOptions options,current;FVM_ExecutionStats stats;unsigned long native;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_execution_options_init(&options)==FVM_OK && options.mode==FVM_EXECUTION_INTERPRETER && !options.flags);
 assert(fvm_function_create_avm1(c,body,sizeof(body),1,params,0,&fn)==FVM_OK);fvm_value_number(&arg,20);
 assert(fvm_function_invoke_options(c,fn,0,1,&arg,&out,&options)==FVM_OK && out.number==42);stats.struct_size=sizeof(stats);assert(fvm_execution_get_stats(c,&stats)==FVM_OK && stats.native_entries==0 && stats.interpreter_entries==1);
 options.mode=FVM_EXECUTION_JIT;assert(fvm_function_invoke_options(c,fn,0,1,&arg,&out,&options)==FVM_OK && out.number==42 && fvm_function_invoke_options(c,fn,0,1,&arg,&out,0)==FVM_OK && out.number==42);
 assert(fvm_execution_get_stats(c,&stats)==FVM_OK);native=fvm_jit_available()?2:0;assert(stats.native_entries==native);if(native)assert(stats.compiled_bodies==1 && stats.cache_hits==1);
 options.mode=FVM_EXECUTION_INTERPRETER;assert(fvm_function_invoke_options(c,fn,0,1,&arg,&out,&options)==FVM_OK && out.number==42 && fvm_execution_get_stats(c,&stats)==FVM_OK && stats.native_entries==native);
 fvm_value_number(&arg,20);assert(fvm_set_global(c,"x",&arg)==FVM_OK);options.mode=FVM_EXECUTION_JIT;assert(fvm_avm1_execute_options(c,body,sizeof(body),&options)==FVM_OK && fvm_execution_get_stats(c,&stats)==FVM_OK && stats.native_entries==native+(native?1:0));
 assert(fvm_function_create_avm1(c,unsupported,sizeof(unsupported),0,0,0,&fallback)==FVM_OK && fvm_function_invoke_options(c,fallback,0,0,0,&out,0)==FVM_OK && out.number==42 && fvm_function_invoke_options(c,fallback,0,0,0,&out,0)==FVM_OK && out.number==42 && fvm_execution_get_stats(c,&stats)==FVM_OK && stats.fallbacks>=2);
 current.struct_size=sizeof(current);assert(fvm_execution_get_options(c,&current)==FVM_OK && current.mode==FVM_EXECUTION_JIT);
 options.flags=8;fvm_value_number(&out,99);assert(fvm_function_invoke_options(c,fn,0,0,0,&out,&options)==FVM_INVALID_ARGUMENT && out.number==99);options.flags=0;options.struct_size=0;assert(fvm_avm1_execute_options(c,body,sizeof(body),&options)==FVM_INVALID_ARGUMENT);
 assert(fvm_execution_set_mode(c,(FVM_ExecutionMode)3,0)==FVM_INVALID_ARGUMENT && fvm_execution_set_mode(c,FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
