#include "flashvm/avm1.h"
#include "flashvm/value.h"
#include <stdio.h>


int main(void)
{ /* Push 4, 4; Equals; Not; PushDuplicate; StackSwap; Pop; SetVariable. */
    static const unsigned char code[] = {0x96, 10,   0,    7, 4, 0, 0,   0,   7,   4,   0,   0,   0, 0x0e, 0x12, 0x4c,
                                         0x4d, 0x17, 0x96, 8, 0, 0, 'r', 'e', 's', 'u', 'l', 't', 0, 0x4d, 0x1d, 0};
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *ctx = 0;
    FVM_Value v;
    if (fvm_config_init(&cfg) != FVM_OK || fvm_runtime_create(&cfg, &rt) != FVM_OK)
    {
        return 1;
    }
    if (fvm_context_create(rt, &ctx) != FVM_OK)
    {
        return 2;
    }
    if (fvm_avm1_execute(ctx, code, sizeof(code)) != FVM_OK)
    {
        return 3;
    }
    if (fvm_get_global(ctx, "result", &v) != FVM_OK || v.type != FVM_VALUE_BOOLEAN || v.boolean)
    {
        return 4;
    }
    fvm_context_destroy(ctx);
    fvm_runtime_destroy(rt);
    puts("AVM1 stack operations OK");
    return 0;
}
