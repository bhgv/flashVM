#include "flashvm/avm1.h"
#include "flashvm/value.h"
#include <stdio.h>
static FVM_Result native(FVM_Context *c,FVM_Object *o,unsigned int n,const FVM_Value *v,FVM_Value *r,void *u) {
 (void)c;(void)o;(void)u;
 if(n!=1 || v[0].type!=FVM_VALUE_NUMBER || v[0].number!=42) return FVM_ERROR;
 fvm_value_number(r,1);return FVM_OK;
}
int main(void) {
 /* FWS v8, 1x1 RECT, frame rate 12, one DoAction and End tag. */
 static unsigned char swf[]={
 'F','W','S',8,0,0,0,0,0x28,0,0,0,0x00,0x0c,1,0,
 0,0,0,0,0,0
 };
 /* Reuse a minimal action: push 42, push argc=1 + native, call, pop, end. */
 static const unsigned char action[]={
 0x96,5,0,7,42,0,0,0,
 0x96,0x0d,0,7,1,0,0,0,0,'n','a','t','i','v','e',0,
 0x3d,0x17,0
 };
 unsigned char bytes[128]; size_t n=16+2+sizeof(action)+2;
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *ctx=0;
 unsigned int header=(12<<6)|(unsigned int)sizeof(action);
 if(n>sizeof(bytes)) return 1;
 {
  size_t i;for(i=0;i<16;++i) bytes[i]=swf[i];
  bytes[4]=(unsigned char)n;bytes[5]=(unsigned char)(n>>8);
  bytes[16]=(unsigned char)header;bytes[17]=(unsigned char)(header>>8);
  for(i=0;i<sizeof(action);++i) bytes[18+i]=action[i];
  bytes[18+sizeof(action)]=0;bytes[19+sizeof(action)]=0;
 }
 if(fvm_config_init(&cfg)!=FVM_OK || fvm_runtime_create(&cfg,&rt)!=FVM_OK) return 2;
 if(fvm_context_create(rt,&ctx)!=FVM_OK) return 3;
 if(fvm_register_function(ctx,"native",native,0)!=FVM_OK) return 4;
 if(fvm_load_memory(ctx,bytes,n)!=FVM_OK) return 5;
 if(fvm_swf_execute_actions(ctx)!=FVM_OK) return 6;
 fvm_context_destroy(ctx);fvm_runtime_destroy(rt);
 puts("SWF DoAction native call OK");return 0;
}
