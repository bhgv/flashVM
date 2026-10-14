/* Binary string service failures preserve output and both input-root snapshots. */
#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"
#include "../src/avm1/encoding_constants.h"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);FVM_Value a,b,out;fvm_value_string_bytes(&a,"a\0b",3);fvm_value_string_bytes(&b,"c\0d",3);
 for(unsigned i=0;i<4;++i){bool success=false;unsigned failures=0;for(long boundary=0;boundary<1000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;fvm_value_number(&out,99);allowance=boundary;FVM_Result r=i?fvm_exec_string_compare(c,i==1?FVM_EXEC_EQUAL:i==2?FVM_EXEC_LESS:FVM_EXEC_GREATER,&a,&b,&out):fvm_exec_string_add(c,&a,&b,&out);allowance=-1;assert(c->gc_frames==roots);if(r==FVM_OK){assert(i?out.type==FVM_VALUE_BOOLEAN:(out.type==FVM_VALUE_STRING && fvm_value_string_size(&out)==6));success=true;}else{assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==99);++failures;}assert(fvm_gc_collect(c,0)==FVM_OK);}assert(success && failures);}
 unsigned char swf[]={'F','W','S',6,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);fvm_value_undefined(&a);fvm_value_undefined(&b);c->string_depth=FVM_AVM1_MAX_SCRIPT_DEPTH;assert(fvm_exec_string_add(c,&a,&b,&out)==FVM_OK && out.type==FVM_VALUE_STRING && fvm_value_string_size(&out)==0 && c->string_depth==FVM_AVM1_MAX_SCRIPT_DEPTH);c->string_depth=0;
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
