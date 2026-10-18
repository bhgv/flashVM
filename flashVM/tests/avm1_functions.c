#include "flashvm/avm1.h"
#include "flashvm/value.h"
#include <stdio.h>
int main(void) {
    /* DefineFunction answer() { return 42; } ; x = answer(); */
    static const unsigned char code[]={
      0x9b,0x14,0x00,'a','n','s','w','e','r',0,0,0,9,0,
      0x96,5,0,7,42,0,0,0,0x3e,
      0x96,0x10,0,0,'x',0,7,0,0,0,0,0,'a','n','s','w','e','r',0,
      0x3d,0x1d,0
    };
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *ctx=0; FVM_Value v;
    if(fvm_config_init(&cfg)!=FVM_OK || fvm_runtime_create(&cfg,&rt)!=FVM_OK) return 1;
    if(fvm_context_create(rt,&ctx)!=FVM_OK) return 2;
    if(fvm_avm1_execute(ctx,code,sizeof(code))!=FVM_OK) return 3;
    if(fvm_get_global(ctx,"x",&v)!=FVM_OK || v.type!=FVM_VALUE_NUMBER || v.number!=42) return 4;
    fvm_context_destroy(ctx); fvm_runtime_destroy(rt);
    puts("named ActionScript function OK"); return 0;
}
