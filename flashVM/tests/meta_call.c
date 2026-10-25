#include <stdio.h>
#include <stdlib.h>
#include "flashvm/meta_call.h"
#include "flashvm/metamethod.h"
#include "flashvm/table.h"


static FVM_Result
callback(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *argv, FVM_Value *out, void *userdata)
{
    FVM_Value a, b;
    (void)userdata;
    if (argc != 3 || argv[0].object != self || argv[1].type != FVM_VALUE_OBJECT || argv[2].type != FVM_VALUE_OBJECT)
    {
        return FVM_ERROR;
    }
    if (fvm_table_get_index(c, argv[2].object, 0, &a) != FVM_OK ||
        fvm_table_get_index(c, argv[2].object, 1, &b) != FVM_OK)
    {
        return FVM_ERROR;
    }
    if (a.type != FVM_VALUE_NUMBER || b.type != FVM_VALUE_NUMBER)
    {
        return FVM_ERROR;
    }
    fvm_value_number(out, a.number + b.number);
    return FVM_OK;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *obj = 0, *meta = 0;
    FVM_Value args[2], out;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &obj) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &meta) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_table_set_metatable(c, obj, meta) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_meta_register(c, meta, "__call", callback, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&args[0], 19);
    fvm_value_number(&args[1], 23);
    if (!(fvm_meta_call(c, obj, 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(out.type == FVM_VALUE_NUMBER && out.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
