/* C89 embedding demo: object globals and a registered native function.
 * SWF/ActionScript execution is not yet implemented.
 */
#include "flashvm/flashvm.h"
#include "flashvm/value.h"
#include <stdio.h>
static FVM_Result sum(FVM_Context *ctx, FVM_Object *self, unsigned int argc,
    const FVM_Value *args, FVM_Value *out, void *userdata)
{
    (void)ctx; (void)self; (void)userdata;
    if (argc != 2 || args[0].type != FVM_VALUE_NUMBER ||
        args[1].type != FVM_VALUE_NUMBER) return FVM_INVALID_ARGUMENT;
    fvm_value_number(out, args[0].number + args[1].number);
    return FVM_OK;
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *ctx = 0;
    FVM_Object *obj = 0;
    FVM_Value value, got, args[2];
    int status = 1;
    if (fvm_config_init(&cfg) != FVM_OK) return 1;
    if (fvm_runtime_create(&cfg, &rt) != FVM_OK) return 1;
    if (fvm_context_create(rt, &ctx) != FVM_OK) goto done;
    /* VM owns this object until context destruction. */
    if (fvm_object_create(ctx, &obj) != FVM_OK) goto done;
    if (fvm_value_string(&value, "Hello from C host") != FVM_OK) goto done;
    if (fvm_object_set(ctx, obj, "message", &value) != FVM_OK) goto done;
    /* Publish the object to the context's global namespace. */
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = obj;
    if (fvm_set_global(ctx, "hostObject", &value) != FVM_OK) goto done;
    if (fvm_get_global(ctx, "hostObject", &got) != FVM_OK) goto done;
    if (got.type != FVM_VALUE_OBJECT || got.object != obj) goto done;
    if (fvm_object_get(ctx, obj, "message", &got) != FVM_OK) goto done;
    if (got.type != FVM_VALUE_STRING) goto done;
    printf("%s\n", got.string); /* Borrowed string; don't free it. */
    if (fvm_register_function(ctx, "nativeSum", sum, 0) != FVM_OK) goto done;
    fvm_value_number(&args[0], 10);
    fvm_value_number(&args[1], 20);
    if (fvm_call(ctx, "nativeSum", 2, args, &got) != FVM_OK) goto done;
    if (got.type != FVM_VALUE_NUMBER || got.number != 30) goto done;
    printf("nativeSum(10, 20) = %.0f\n", got.number);
    status = 0;
done:
    if (ctx) fvm_context_destroy(ctx);
    fvm_runtime_destroy(rt);
    return status;
}
