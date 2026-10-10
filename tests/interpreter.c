#include "flashvm/avm1.h"
#include "flashvm/value.h"
#include <stdio.h>
#include <string.h>
static FVM_Result native(FVM_Context *c,FVM_Object *o,unsigned int n,const FVM_Value *args,FVM_Value *out,void *u) {
    (void)c;(void)o;(void)u;
    if(n!=1 || args[0].type!=FVM_VALUE_NUMBER || args[0].number!=30) return FVM_ERROR;
    fvm_value_number(out,99);return FVM_OK;
}
int main(void) {
    /* push 'x', 10; setvariable; push 'x'; getvariable; push 20; add;
       push arg count=1, function name='native'; callfunction; pop; end */
    static const unsigned char code[]={
      0x96,0x08,0,0,'x',0,7,10,0,0,0,0x1d,
      0x96,3,0,0,'x',0,0x1c,
      0x96,5,0,7,20,0,0,0,0x0a,
      0x96,0x0d,0,7,1,0,0,0,0,'n','a','t','i','v','e',0,
      0x3d,0x17,0};
    FVM_Config cfg; FVM_Runtime *rt=0;FVM_Context *ctx=0;FVM_Value v;
    if(fvm_config_init(&cfg)!=FVM_OK || fvm_runtime_create(&cfg,&rt)!=FVM_OK) return 1;
    if(fvm_context_create(rt,&ctx)!=FVM_OK) return 2;
    if(fvm_register_function(ctx,"native",native,0)!=FVM_OK) return 3;
    if(fvm_avm1_execute(ctx,code,sizeof(code))!=FVM_OK) return 4;
    if(fvm_get_global(ctx,"x",&v)!=FVM_OK || v.type!=FVM_VALUE_NUMBER || v.number!=10) return 5;
    fvm_context_destroy(ctx);fvm_runtime_destroy(rt);
    puts("AVM1 execution OK");return 0;
}
