#include "flashvm/avm1.h"
#include "flashvm/value.h"
int main(void) {
 /* Push 0, 'Object'; NewObject; Push 'answer', 42; SetMember. */
 static const unsigned char code[]={0x96,0x0d,0,7,0,0,0,0,0,'O','b','j','e','c','t',0,0x40,0x96,0x0d,0,0,'a','n','s','w','e','r',0,7,42,0,0,0,0x4f,0};
 FVM_Config cfg;FVM_Runtime *r=0;FVM_Context *c=0;FVM_Result result;
 if(fvm_config_init(&cfg)!=FVM_OK || fvm_runtime_create(&cfg,&r)!=FVM_OK || fvm_context_create(r,&c)!=FVM_OK) return 1;
 result=fvm_avm1_execute(c,code,sizeof(code));fvm_context_destroy(c);fvm_runtime_destroy(r);
 return result==FVM_OK?0:2;
}
