/* Native backend acceptance: independent VM owners, source lifetime and actual entry counters. */
#include "flashvm/jit.h"
#include "flashvm/standard.h"
#include "flashvm/execution_mode.h"
#include <cassert>
#include <cstring>
static void execute(FVM_Context *context,FVM_Object *function)
{
    FVM_Value value;
    FVM_ExecutionStats stats;stats.struct_size=sizeof(stats);
    assert(fvm_function_invoke(context,function,0,0,0,&value)==FVM_OK);
    assert(value.type==FVM_VALUE_NUMBER && value.number==42);
    assert(fvm_execution_get_stats(context,&stats)==FVM_OK && stats.native_entries);
}
int main()
{
    if(!fvm_jit_available())return 77;
    const unsigned char bytes[]={0x96,5,0,7,42,0,0,0,0x3e,0};
    FVM_JitCode *code[2]={0,0};
    FVM_Result status=fvm_jit_compile(bytes,sizeof(bytes),&code[0]);
    /* The first mapping can legitimately be denied by host signing policy. */
    if(status==FVM_UNSUPPORTED){assert(!code[0]);return 77;}
    assert(status==FVM_OK && code[0]);
    /* Two simultaneous bodies must not require a second MAP_JIT region. */
    assert(fvm_jit_compile(bytes,sizeof(bytes),&code[1])==FVM_OK && code[1]);
    FVM_Config config;assert(fvm_config_init(&config)==FVM_OK);
    FVM_Runtime *runtime[2]={0,0};FVM_Context *context[2]={0,0};FVM_Object *function[2]={0,0};
    for(unsigned i=0;i<2;++i)
    {
        assert(fvm_runtime_create(&config,&runtime[i])==FVM_OK);
        assert(fvm_context_create(runtime[i],&context[i])==FVM_OK);
        assert(fvm_standard_install(context[i])==FVM_OK);
        assert(fvm_execution_set_mode(context[i],FVM_EXECUTION_MANUAL,0)==FVM_OK);
        assert(fvm_function_create_avm1(context[i],bytes,sizeof(bytes),0,0,0,&function[i])==FVM_OK);
        assert(fvm_function_set_body(context[i],function[i],fvm_jit_body,code[i],0)==FVM_OK);
        execute(context[i],function[i]);
    }
    assert(fvm_function_set_body(context[0],function[0],0,0,0)==FVM_OK);
    assert(fvm_jit_destroy(code[0])==FVM_OK);code[0]=0;
    fvm_context_destroy(context[0]);fvm_runtime_destroy(runtime[0]);
    /* Teardown of one VM must not invalidate another VM's live native slot or source copy. */
    execute(context[1],function[1]);
    const void *source=0;size_t size=0;
    assert(fvm_jit_source(code[1],&source,&size)==FVM_OK && size==sizeof(bytes));
    assert(!std::memcmp(source,bytes,size));
    FVM_JitCode *replacement=0;
    assert(fvm_jit_compile(bytes,sizeof(bytes),&replacement)==FVM_OK && replacement);
    assert(fvm_function_set_body(context[1],function[1],fvm_jit_body,replacement,0)==FVM_OK);
    assert(fvm_jit_destroy(code[1])==FVM_OK);code[1]=0;
    execute(context[1],function[1]);
    assert(fvm_function_set_body(context[1],function[1],0,0,0)==FVM_OK);
    assert(fvm_jit_destroy(replacement)==FVM_OK);
    fvm_context_destroy(context[1]);fvm_runtime_destroy(runtime[1]);
    return 0;
}
