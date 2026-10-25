#include "flashvm/avm1.h"
#include "flashvm/value.h"
#include <stdio.h>

static FVM_Object *expected;


static FVM_Result
native_method(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *argv, FVM_Value *out, void *user)
{
    (void)c;
    (void)user;
    if (self != expected || argc != 1 || argv[0].type != FVM_VALUE_NUMBER || argv[0].number != 7)
    {
        return FVM_ERROR;
    }
    fvm_value_number(out, 42);
    return FVM_OK;
}


int main(void)
{ /* Push "result", 7, 1, "receiver", GetVariable, "compute", CallMethod, SetVariable, End. */
    static const unsigned char code[] = {0x96, 8, 0, 0,   'r', 'e',  's', 'u', 'l', 't', 0,   0x96, 5,    0,
                                         7,    7, 0, 0,   0,   0x96, 5,   0,   7,   1,   0,   0,    0,    0x96,
                                         10,   0, 0, 'r', 'e', 'c',  'e', 'i', 'v', 'e', 'r', 0,    0x1c, 0x96,
                                         9,    0, 0, 'c', 'o', 'm',  'p', 'u', 't', 'e', 0,   0x52, 0x1d, 0};
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
    if (fvm_object_create(ctx, &expected) != FVM_OK)
    {
        return 3;
    }
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = expected;
    if (fvm_set_global(ctx, "receiver", &v) != FVM_OK)
    {
        return 4;
    }
    if (fvm_register_function(ctx, "compute", native_method, 0) != FVM_OK)
    {
        return 5;
    }
    if (fvm_avm1_execute(ctx, code, sizeof(code)) != FVM_OK)
    {
        return 6;
    }
    if (fvm_get_global(ctx, "result", &v) != FVM_OK || v.type != FVM_VALUE_NUMBER || v.number != 42)
    {
        return 7;
    }
    fvm_context_destroy(ctx);
    fvm_runtime_destroy(rt);
    puts("CallMethod native receiver OK");
    return 0;
}
