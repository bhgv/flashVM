#include <stdio.h>
#include <stdlib.h>
#include "flashvm/value.h"
#include <string.h>


static FVM_Result add(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *a, FVM_Value *out, void *user)
{
    (void)c;
    (void)self;
    (void)user;
    if (n != 2 || a[0].type != FVM_VALUE_NUMBER || a[1].type != FVM_VALUE_NUMBER)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_number(out, a[0].number + a[1].number);
    return FVM_OK;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *ctx = 0;
    FVM_Object *obj = 0;
    FVM_Value v, got, args[2];
    char buffer[32] = "hello";
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
    if (!(fvm_context_create(rt, &ctx) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(ctx, &obj) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string(&v, buffer) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(ctx, obj, "message", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    strcpy(buffer, "changed");
    if (!(fvm_object_get(ctx, obj, "message", &got) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(strcmp(got.string, "hello") == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = obj;
    if (!(fvm_set_global(ctx, "object", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(ctx, "object", &got) == FVM_OK && got.object == obj))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_register_function(ctx, "add", add, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&args[0], 12);
    fvm_value_number(&args[1], 30);
    if (!(fvm_call(ctx, "add", 2, args, &got) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(got.type == FVM_VALUE_NUMBER && got.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(ctx);
    fvm_runtime_destroy(rt);
    return 0;
}
