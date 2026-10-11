#include "flashvm/auxlib.h"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <limits.h>
#include <string.h>

static int add(FVM_Stack *stack)
{
    double a, b;
    FVM_Result result;
    result = fvm_stack_checknumber(stack, 1, &a);
    if (result != FVM_OK)
        return result;
    result = fvm_stack_checknumber(stack, 2, &b);
    if (result != FVM_OK)
        return result;
    result = fvm_stack_pushnumber(stack, a + b);
    return result == FVM_OK ? 1 : result;
}

static int empty(FVM_Stack *stack)
{
    (void)stack;
    return 0;
}

static int invalid_count(FVM_Stack *stack)
{
    (void)stack;
    return 2;
}

int main(void)
{
    /* SetVariable("answer", add(20, 22)), with arguments pushed in AVM1 order. */
    static const unsigned char actions[] = {0x96, 28, 0, 0,   'a', 'n', 's', 'w',  'e',  'r', 0, 7,
                                            22,   0,  0, 0,   7,   20,  0,   0,    0,    7,   2, 0,
                                            0,    0,  0, 'a', 'd', 'd', 0,   0x3d, 0x1d, 0};
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Stack *stack = 0;
    FVM_Value args[2], out;
    double number;
    char text[] = "owned";
    const char *string = 0;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_stack_create(context, &stack) == FVM_OK);
    assert(fvm_stack_context(stack) == context);
    assert(fvm_stack_settop(stack, 3) == FVM_OK);
    assert(fvm_stack_get(stack, 3, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_stack_settop(stack, -2) == FVM_OK && fvm_stack_size(stack) == 2);
    assert(fvm_stack_settop(stack, INT_MIN) == FVM_INVALID_ARGUMENT);
    assert(fvm_stack_clear(stack) == FVM_OK);
    assert(fvm_stack_pushnumber(stack, 1) == FVM_OK);
    assert(fvm_stack_pushnumber(stack, 2) == FVM_OK);
    assert(fvm_stack_pushnumber(stack, 3) == FVM_OK);
    assert(fvm_stack_insert(stack, 1) == FVM_OK);
    assert(fvm_stack_checknumber(stack, 1, &number) == FVM_OK && number == 3);
    assert(fvm_stack_remove(stack, -2) == FVM_OK);
    assert(fvm_stack_replace(stack, 1) == FVM_OK);
    assert(fvm_stack_checknumber(stack, 1, &number) == FVM_OK && number == 2);
    assert(fvm_stack_pushvalue(stack, INT_MIN) == FVM_INVALID_ARGUMENT);
    assert(fvm_stack_clear(stack) == FVM_OK);
    assert(fvm_stack_pushstring(stack, text) == FVM_OK);
    text[0] = 'X';
    assert(fvm_stack_checkstring(stack, -1, &string) == FVM_OK);
    assert(strcmp(string, "owned") == 0);
    assert(fvm_stack_checknumber(stack, -1, &number) == FVM_INVALID_ARGUMENT);
    assert(fvm_stack_clear(stack) == FVM_OK);
    assert(fvm_stack_newtable(stack) == FVM_OK);
    assert(fvm_stack_pushnumber(stack, 42) == FVM_OK);
    assert(fvm_stack_setfield(stack, -2, "value") == FVM_OK);
    assert(fvm_gc_collect(context, 0) == FVM_OK);
    assert(fvm_stack_getfield(stack, 1, "value") == FVM_OK);
    assert(fvm_stack_checknumber(stack, -1, &number) == FVM_OK && number == 42);
    assert(fvm_stack_pop(stack, 1) == FVM_OK);
    assert(fvm_stack_setglobal(stack, "table") == FVM_OK);
    assert(fvm_stack_getglobal(stack, "table") == FVM_OK);
    assert(fvm_stack_clear(stack) == FVM_OK);

    assert(fvm_stack_register_cfunction(context, "add", add, 0) == FVM_OK);
    fvm_value_number(args, 20);
    fvm_value_number(args + 1, 22);
    assert(fvm_call(context, "add", 2, args, &out) == FVM_OK && out.number == 42);
    assert(fvm_stack_call(context, "add", 2, args, &out) == FVM_OK && out.number == 42);
    assert(fvm_call(context, "add", 0, 0, &out) == FVM_INVALID_ARGUMENT);
    assert(fvm_stack_register_cfunction(context, "empty", empty, 0) == FVM_OK);
    assert(fvm_call(context, "empty", 2, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_stack_register_cfunction(context, "bad", invalid_count, 0) == FVM_OK);
    assert(fvm_call(context, "bad", 0, 0, &out) == FVM_ERROR);
    assert(fvm_avm1_execute(context, actions, sizeof(actions)) == FVM_OK);
    assert(fvm_get_global(context, "answer", &out) == FVM_OK && out.number == 42);
    fvm_stack_destroy(stack);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
