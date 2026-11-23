/* Copyright (c) 2026 bhgv
 * EN: Exercise value-only managed calls in both VM directions.
 * RU: Проверка управляемых вызовов по значению в обе стороны.
 */
#include "managed-test.h"
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "styxrpc/managed_module.h"
#include "drivers/posix/fd.h"
#include <stdio.h>
#include <string.h>

#define REAL_SIGNATURE UINT32_C(0xdf75d234) /* Limbo fn(real, string): (real, string). */
#define ECHO_SIGNATURE UINT32_C(0x8e406ff6) /* Limbo fn(string, array of byte): (string, array of byte). */
#define ECHO_COUNT 600U     /* Large enough to exceed the negotiated Styx write size. */
#define ECHO_WAIT_MS 3000U  /* Bounded reply polling. */

#define ACTION_END 0x00           /* End the function's action block. */
#define ACTION_GET_VARIABLE 0x1c  /* Resolve a bound parameter. */
#define ACTION_SET_VARIABLE 0x1d  /* Store the checked script result. */
#define ACTION_CALL_FUNCTION 0x3d /* Call the registered module function. */
#define ACTION_RETURN 0x3e        /* Return the result Array. */
#define ACTION_INIT_ARRAY 0x42    /* Build the ordered result tuple as an AS Array. */
#define ACTION_PUSH 0x96          /* Push typed operands. */
#define PUSH_STRING 0x00          /* Encoded NUL-terminated name. */
#define PUSH_INTEGER 0x07         /* Little-endian integer operand. */

static const StyxRpcSchema real_shape = {"(fs)", "((fs))"}; /* Managed binary64 and text. */
static const StyxRpcSchema shape = {"(sa[y])", "((sa[y]))"}; /* Same compiled shape on both endpoints. */


/* EN: Define a real two-argument AVM1 implementation that returns the two values.
 * RU: Определить настоящую функцию AVM1, возвращающую два переданных значения. */
static int define_echo(FVM_Context *context)
{
    static const unsigned char code[] = {
        ACTION_PUSH, 3, 0, PUSH_STRING, 'b', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 3, 0, PUSH_STRING, 's', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 5, 0, PUSH_INTEGER, 2, 0, 0, 0, ACTION_INIT_ARRAY, ACTION_RETURN, ACTION_END
    };
    const char *params[] = {"s", "b"};
    FVM_Object *function = NULL;
    FVM_Value value;
    if (fvm_function_create_avm1(context, code, sizeof(code), 2, params, NULL, &function) != FVM_OK)
    {
        return 0;
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = function;
    return fvm_set_global(context, "echo", &value) == FVM_OK;
}


/* EN/RU: See managed.h. / См. managed.h. */
int test_managed_serve(FVM_Context *context, int fd)
{
    unsigned char buffer[STYXRPC_MANAGED_LIMIT];
    FVM_StyxRpcManagedExport binding = {context, "echo", shape, buffer, sizeof(buffer)};
    FVM_StyxRpcManagedExport number = {context, "echo", real_shape, buffer, sizeof(buffer)};
    StyxRpcManagedExport descriptors[2];
    StyxRpcManagedModule module;
    if (!define_echo(context) ||
        fvm_styxrpc_export_managed(&binding, "echo", ECHO_SIGNATURE, descriptors) != FVM_OK ||
        fvm_styxrpc_export_managed(&number, "number", REAL_SIGNATURE, descriptors + 1) != FVM_OK ||
        styxrpc_managed_module_init(&module, "flash-values.dis", descriptors, 2) != STYXRPC_OK)
    {
        fputs("Managed FlashVM publication failed\n", stderr);
        return 0;
    }
    RemotePhy *phy = phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE);
    StyxRpcServer *server = styxrpc_server_create(phy, &styxrpc_managed_module_tree, &module);
    while (styxrpc_server_step(server))
    {
    }
    int ok = server != NULL;
    styxrpc_server_destroy(server);
    return ok;
}


/* EN: Require an own returned tuple with intact counted UTF-8 and changed copied bytes.
 * RU: Проверить возвращённый кортеж со строкой заданной длины и изменённой копией байтов. */
static int check_result(FVM_Context *context, const FVM_Value *result, const char *text, size_t size)
{
    FVM_Value string, array, item;
    return result->type == FVM_VALUE_OBJECT &&
        fvm_object_get(context, result->object, "0", &string) == FVM_OK &&
        string.type == FVM_VALUE_STRING && fvm_value_string_size(&string) == size &&
        !memcmp(string.string, text, size) &&
        fvm_object_get(context, result->object, "1", &array) == FVM_OK && array.type == FVM_VALUE_OBJECT &&
        fvm_object_get(context, array.object, "length", &item) == FVM_OK &&
        item.type == FVM_VALUE_NUMBER && item.number == ECHO_COUNT &&
        fvm_object_get(context, array.object, "1", &item) == FVM_OK &&
        item.type == FVM_VALUE_NUMBER && item.number == 99;
}


/* EN: Verify the current script error text and clear it on the owner.
 * RU: Сверить текст текущего исключения и снять его в потоке владельца. */
static int check_error(FVM_Context *context, const char *message)
{
    FVM_Value error;
    return fvm_exception_get(context, &error) == FVM_OK && error.type == FVM_VALUE_STRING &&
        !strcmp(error.string, message) && fvm_exception_clear(context) == FVM_OK;
}


/* EN: Check a tuple of two nil leaves mapped to ActionScript null.
 * RU: Проверить кортеж из двух nil, отображённых в null ActionScript. */
static int check_nil(FVM_Context *context, const FVM_Value *value)
{
    FVM_Value item;
    return value->type == FVM_VALUE_OBJECT &&
        fvm_object_get(context, value->object, "0", &item) == FVM_OK && item.type == FVM_VALUE_NULL &&
        fvm_object_get(context, value->object, "1", &item) == FVM_OK && item.type == FVM_VALUE_NULL;
}


/* EN: Route the same imported binding to the real export and verify signed zero.
 * RU: Направить ту же привязку на экспорт real и сверить знак нуля. */
static int real_call(FVM_Context *context, StyxRpcManagedTarget *target)
{
    FVM_Value args[2], result, field;
    uint64_t received;
    target->name = "number";
    target->signature = REAL_SIGNATURE;
    target->schema = real_shape;
    fvm_value_number(args, -0.0);
    if (fvm_value_string(args + 1, "number") != FVM_OK ||
        fvm_call_bytes(context, "echo", 4, 2, args, &result) != FVM_OK ||
        result.type != FVM_VALUE_OBJECT ||
        fvm_object_get(context, result.object, "0", &field) != FVM_OK ||
        field.type != FVM_VALUE_NUMBER)
    {
        return 0;
    }
    memcpy(&received, &field.number, sizeof(received));
    return received == UINT64_C(0x8000000000000000) &&
        fvm_object_get(context, result.object, "1", &field) == FVM_OK &&
        field.type == FVM_VALUE_STRING && !strcmp(field.string, "number");
}


/* EN/RU: See managed.h. / См. managed.h. */
int test_managed_call(FVM_Context *context, StyxRpcManagedTarget *target)
{
    const char input[] = {'A', 0, (char)0xf0, (char)0x9f, (char)0x8c, (char)0x8d};
    const char expected[] = {'A', 0, (char)0xf0, (char)0x9f, (char)0x8c, (char)0x8d, '!'};
    static const unsigned char caller[] = {
        ACTION_PUSH, 6, 0, PUSH_STRING, 'r', 0, PUSH_STRING, 'b', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 3, 0, PUSH_STRING, 's', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 11, 0, PUSH_INTEGER, 2, 0, 0, 0, PUSH_STRING, 'e', 'c', 'h', 'o', 0,
        ACTION_CALL_FUNCTION, ACTION_SET_VARIABLE, ACTION_END
    };
    FVM_Value values[ECHO_COUNT], args[2], result, item;
    target->module = "mounted/limbo.dis";
    target->name = "echo";
    target->signature = ECHO_SIGNATURE;
    target->schema = shape;
    target->pid = TEST_MANAGED_PID;
    target->wait_ms = ECHO_WAIT_MS;
    for (unsigned i = 0; i < ECHO_COUNT; ++i)
    {
        fvm_value_number(values + i, i % 256);
    }
    fvm_value_number(values, 42);
    int ok = fvm_styxrpc_import_managed(context, "echo", target) == FVM_OK &&
        fvm_value_string_bytes(args, input, sizeof(input)) == FVM_OK &&
        fvm_exec_literal_array(context, ECHO_COUNT, values, args + 1) == FVM_OK &&
        fvm_set_global(context, "s", args) == FVM_OK &&
        fvm_set_global(context, "b", args + 1) == FVM_OK &&
        fvm_avm1_execute(context, caller, sizeof(caller)) == FVM_OK &&
        fvm_get_global(context, "r", &result) == FVM_OK &&
        check_result(context, &result, expected, sizeof(expected)) &&
        fvm_object_get(context, args[1].object, "1", &item) == FVM_OK && item.number == 1;
    if (ok)
    {
        fvm_value_string(args, "reject");
        ok = fvm_call_bytes(context, "echo", 4, 2, args, &result) == FVM_EXCEPTION &&
            check_error(context, "Styx RPC remote function failed") &&
            fvm_value_string_bytes(args, input, sizeof(input)) == FVM_OK &&
            fvm_call_bytes(context, "echo", 4, 2, args, &result) == FVM_OK &&
            check_result(context, &result, expected, sizeof(expected));
    }
    if (ok)
    {
        target->pid = 0;
        ok = fvm_call_bytes(context, "echo", 4, 2, args, &result) == FVM_EXCEPTION &&
            check_error(context, "Styx RPC call was not sent");
        target->pid = TEST_MANAGED_PID;
    }
    if (ok)
    {
        strcpy(target->schema.result, "((sa[i]))");
        ok = fvm_call_bytes(context, "echo", 4, 2, args, &result) == FVM_EXCEPTION &&
            check_error(context, "Styx RPC call was not sent");
        target->schema = shape;
    }
    if (ok)
    {
        fvm_value_undefined(args);
        fvm_value_undefined(args + 1);
        args[0].type = FVM_VALUE_NULL;
        args[1].type = FVM_VALUE_NULL;
        ok = fvm_call_bytes(context, "echo", 4, 2, args, &result) == FVM_OK &&
            check_nil(context, &result);
    }
    if (ok)
    {
        ok = real_call(context, target);
    }
    if (!ok && fvm_exception_get(context, &item) == FVM_OK && item.type == FVM_VALUE_STRING)
    {
        fprintf(stderr, "Managed FlashVM import failed: %s\n", item.string);
    }
    target->session = NULL;
    return ok;
}
