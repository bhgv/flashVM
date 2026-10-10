/* C89 native extension demo. This is statically registered, not a DLL. */
#include "flashvm/flashvm.h"
#include "flashvm/value.h"
#include <stdio.h>
static FVM_Result make_point(FVM_Context *ctx, FVM_Object *self,
    unsigned int argc, const FVM_Value *args, FVM_Value *out, void *userdata)
{
    FVM_Object *point = 0;
    FVM_Value coordinate;
    (void)self; (void)userdata;
    if (argc != 2 || args[0].type != FVM_VALUE_NUMBER ||
        args[1].type != FVM_VALUE_NUMBER) return FVM_INVALID_ARGUMENT;
    /* Native C creates an object accessible through the public VM API. */
    if (fvm_object_create(ctx, &point) != FVM_OK) return FVM_OUT_OF_MEMORY;
    fvm_value_number(&coordinate, args[0].number);
    if (fvm_object_set(ctx, point, "x", &coordinate) != FVM_OK) return FVM_ERROR;
    fvm_value_number(&coordinate, args[1].number);
    if (fvm_object_set(ctx, point, "y", &coordinate) != FVM_OK) return FVM_ERROR;
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = point;
    return FVM_OK;
}
/* Called by host; not yet a dynamic plugin entry point. */
static FVM_Result register_extension(FVM_Context *ctx)
{
    return fvm_register_function(ctx, "makePoint", make_point, 0);
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *ctx = 0;
    FVM_Value args[2], got, x, y;
    int status = 1;
    if (fvm_config_init(&cfg) != FVM_OK) return 1;
    if (fvm_runtime_create(&cfg, &rt) != FVM_OK) return 1;
    if (fvm_context_create(rt, &ctx) != FVM_OK) goto done;
    if (register_extension(ctx) != FVM_OK) goto done;
    fvm_value_number(&args[0], 12);
    fvm_value_number(&args[1], 34);
    if (fvm_call(ctx, "makePoint", 2, args, &got) != FVM_OK) goto done;
    if (got.type != FVM_VALUE_OBJECT) goto done;
    if (fvm_object_get(ctx, got.object, "x", &x) != FVM_OK) goto done;
    if (fvm_object_get(ctx, got.object, "y", &y) != FVM_OK) goto done;
    if (x.type != FVM_VALUE_NUMBER || y.type != FVM_VALUE_NUMBER) goto done;
    if (x.number != 12 || y.number != 34) goto done;
    printf("makePoint => (%.0f, %.0f)\n", x.number, y.number);
    status = 0;
done:
    if (ctx) fvm_context_destroy(ctx);
    fvm_runtime_destroy(rt);
    return status;
}
