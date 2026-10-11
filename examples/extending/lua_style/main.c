#include "flashvm/auxlib.h"
#include <stdio.h>

/* Arguments remain on the callback stack. Returning 1 selects the topmost
 * value as the result; the host bridge discards the callback's arguments. */
static int add(FVM_Stack *stack)
{
    double left, right;
    FVM_Result result;
    result = fvm_stack_checknumber(stack, 1, &left);
    if (result != FVM_OK)
        return result;
    result = fvm_stack_checknumber(stack, 2, &right);
    if (result != FVM_OK)
        return result;
    result = fvm_stack_pushnumber(stack, left + right);
    return result == FVM_OK ? 1 : result;
}

int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Value args[2], result;
    int status = 1;
    if (fvm_config_init(&config) != FVM_OK)
        return 1;
    if (fvm_runtime_create(&config, &runtime) != FVM_OK)
        return 1;
    if (fvm_context_create(runtime, &context) != FVM_OK)
        goto done;
    if (fvm_stack_register_cfunction(context, "add", add, 0) != FVM_OK)
        goto done;
    fvm_value_number(&args[0], 20);
    fvm_value_number(&args[1], 22);
    if (fvm_call(context, "add", 2, args, &result) != FVM_OK)
        goto done;
    if (result.type != FVM_VALUE_NUMBER || result.number != 42)
        goto done;
    printf("add(20, 22) = %.0f\n", result.number);
    status = 0;
done:
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return status;
}
