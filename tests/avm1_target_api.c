/* C89 host introspection observes active borrowed clips and cleared state outside execution. */
#include "flashvm/avm1.h"
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include <assert.h>
static unsigned calls;
static FVM_Result trace(FVM_Context *c,FVM_Object *receiver,unsigned n,const FVM_Value *a,FVM_Value *out,void *raw)
{
 FVM_AVM1TargetInfo info;info.struct_size=sizeof(info);assert(receiver && n==1 && a[0].type==FVM_VALUE_STRING);assert(fvm_avm1_get_target_info(c,&info)==FVM_OK && info.active && info.base_clip==(FVM_Object *)raw && info.target_clip==(FVM_Object *)raw);++calls;fvm_value_undefined(out);return FVM_OK;
}
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *root=0;FVM_AVM1TargetInfo info;const unsigned char actions[]={0x96,3,0,0,'x',0,0x26,0};
 assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);info.struct_size=sizeof(info);info.active=7;info.base_clip=0;info.target_clip=0;assert(fvm_avm1_get_target_info(0,&info)==FVM_INVALID_ARGUMENT && info.active==7);assert(fvm_avm1_get_target_info(c,0)==FVM_INVALID_ARGUMENT);info.struct_size=0;assert(fvm_avm1_get_target_info(c,&info)==FVM_INVALID_ARGUMENT && info.active==7);info.struct_size=sizeof(info);assert(fvm_avm1_get_target_info(c,&info)==FVM_OK && !info.active && !info.base_clip && !info.target_clip);
 assert(fvm_standard_install(c)==FVM_OK);assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK);assert(fvm_avm1_set_trace_rooted(c,trace,root,root)==FVM_OK);assert(fvm_avm1_execute_receiver(c,actions,sizeof(actions),root)==FVM_OK && calls==1);assert(fvm_avm1_get_target_info(c,&info)==FVM_OK && !info.active && !info.base_clip && !info.target_clip);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
