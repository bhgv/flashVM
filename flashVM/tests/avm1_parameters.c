#include "flashvm/avm1.h"
#include "flashvm/value.h"
#include <stdio.h>
int main(void) {
    /* DefineFunction echo(n) { return n; } ; result = echo(42); */
    static const unsigned char code[] = {
      0x9b,0x13,0x00,'e','c','h','o',0,1,0,'n',0,8,0,
      0x96,3,0,0,'n',0,0x1c,0x3e,
      0x96,0x18,0x00,0,'r','e','s','u','l','t',0,7,42,0,0,0,
      7,1,0,0,0,0,'e','c','h','o',0,
      0x3d,0x1d,0
    };
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *ctx=0; FVM_Value value;
    if (fvm_config_init(&cfg)!=FVM_OK || fvm_runtime_create(&cfg,&rt)!=FVM_OK) return 1;
    if (fvm_context_create(rt,&ctx)!=FVM_OK) return 2;
    if (fvm_avm1_execute(ctx,code,sizeof(code))!=FVM_OK) return 3;
    if (fvm_get_global(ctx,"result",&value)!=FVM_OK || value.type!=FVM_VALUE_NUMBER || value.number!=42) return 4;
    fvm_context_destroy(ctx); fvm_runtime_destroy(rt);
    puts("AVM1 named function parameter OK"); return 0;
}
