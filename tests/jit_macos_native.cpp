/* Native macOS Intel fixture: availability/policy distinction and source lifetime after execution. */
#include "../ports/mac/code_platform.h"
#include "flashvm/jit.h"
#include "flashvm/standard.h"
#include <cassert>
#include <cstring>
int main()
{
#if FVM_MAC_CODE_SUPPORTED
 const unsigned char bytes[]={0x96,5,0,7,42,0,0,0,0x3e,0};FVM_JitCode *code=0;
 assert(fvm_jit_available() && !std::strcmp(fvm_jit_backend_name(),"macos-x64"));
 FVM_Result result=fvm_jit_compile(bytes,sizeof(bytes),&code);
 /* Host code signing/policy may legitimately deny classic anonymous RX mappings. */
 if(result==FVM_UNSUPPORTED){assert(!code);return 77;}
 assert(result==FVM_OK && code);
 FVM_Config cfg;FVM_Runtime *runtime=0;FVM_Context *context=0;FVM_Object *function=0;FVM_Value value;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK && fvm_context_create(runtime,&context)==FVM_OK);
 assert(fvm_standard_install(context)==FVM_OK && fvm_function_create_avm1(context,bytes,sizeof(bytes),0,0,0,&function)==FVM_OK);
 assert(fvm_function_set_body(context,function,fvm_jit_body,code,0)==FVM_OK);
 assert(fvm_function_invoke(context,function,0,0,0,&value)==FVM_OK && value.type==FVM_VALUE_NUMBER && value.number==42);
 const void *source=0;size_t size=0;assert(fvm_jit_source(code,&source,&size)==FVM_OK && size==sizeof(bytes) && !std::memcmp(source,bytes,size));
 assert(fvm_function_set_body(context,function,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);
 fvm_context_destroy(context);fvm_runtime_destroy(runtime);
#else
 return 77;
#endif
 return 0;
}
