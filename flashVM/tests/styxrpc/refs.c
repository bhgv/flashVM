/* Copyright (c) 2026 bhgv
 * EN: Exercise alias-preserving ADT copy-back between real FlashVM and DisVM.
 * RU: Проверить обратную запись ADT с алиасами между настоящими FlashVM и DisVM.
 */
#include "ref-test.h"
#include "managed-test.h"
#include "flashvm/exception.h"
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "styxrpc/managed_module.h"
#include "drivers/posix/fd.h"
#include <stdio.h>
#include <string.h>

#define REF_SIGNATURE UINT32_C(0x4a4d5ebd) /* Compiled signature of refs.m. */
#define REF_WAIT_MS 3000U /* Fixed hosted reply polling budget. */
#define ACTION_END 0x00 /* End the action body. */
#define ACTION_GET_VARIABLE 0x1c /* Resolve the named parameter. */
#define ACTION_THROW 0x2a         /* Fail after mutating the local reference. */
#define ACTION_RETURN 0x3e        /* Return the changed field. */
#define ACTION_GET_MEMBER 0x4e    /* Read an indexed field on an aliased ref. */
#define ACTION_SET_MEMBER 0x4f    /* Replace an indexed field on the local ref. */
#define ACTION_PUSH 0x96          /* Push typed operands. */
#define PUSH_STRING 0x00          /* NUL-terminated ActionScript name. */
#define PUSH_INTEGER 0x07         /* Encoded integer argument. */

static const StyxRpcSchema schema = {"(sr[sa[y]]r[sa[y]]r[sa[y]])", "(s)"}; /* Resolved refs.m ABI. */


/* EN: Define one real AVM1 implementation on the serving context.
 * RU: Определить реализацию AVM1 в обслуживающем контексте. */
static int define(FVM_Context *context, const char *name, const unsigned char *actions, size_t size)
{
    const char *parameters[] = {"t", "a", "b", "c"};
    FVM_Object *function = NULL;
    FVM_Value value;
    if (fvm_function_create_avm1(context, actions, size, 4, parameters, NULL, &function) != FVM_OK)
    {
        return 0;
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = function;
    return fvm_set_global(context, name, &value) == FVM_OK;
}


/* EN/RU: See ref-test.h. / См. ref-test.h. */
int test_refs_serve(FVM_Context *context, int fd)
{
    /* a[0] = t; return b[0]; */
    static const unsigned char change[] = {
        ACTION_PUSH, 3, 0, PUSH_STRING, 'a', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 5, 0, PUSH_INTEGER, 0, 0, 0, 0,
        ACTION_PUSH, 3, 0, PUSH_STRING, 't', 0, ACTION_GET_VARIABLE, ACTION_SET_MEMBER,
        ACTION_PUSH, 3, 0, PUSH_STRING, 'b', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 5, 0, PUSH_INTEGER, 0, 0, 0, 0, ACTION_GET_MEMBER, ACTION_RETURN, ACTION_END
    };
    /* a[0] = t; throw t; */
    static const unsigned char reject[] = {
        ACTION_PUSH, 3, 0, PUSH_STRING, 'a', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 5, 0, PUSH_INTEGER, 0, 0, 0, 0,
        ACTION_PUSH, 3, 0, PUSH_STRING, 't', 0, ACTION_GET_VARIABLE, ACTION_SET_MEMBER,
        ACTION_PUSH, 3, 0, PUSH_STRING, 't', 0, ACTION_GET_VARIABLE, ACTION_THROW, ACTION_END
    };
    unsigned char buffer[STYXRPC_MANAGED_LIMIT];
    FVM_StyxRpcManagedExport first = {context, "change", schema, buffer, sizeof(buffer)};
    FVM_StyxRpcManagedExport second = {context, "reject", schema, buffer, sizeof(buffer)};
    StyxRpcManagedExport exports[2];
    StyxRpcManagedModule module;
    if (!define(context, "change", change, sizeof(change)) || !define(context, "reject", reject, sizeof(reject)) ||
        fvm_styxrpc_export_managed(&first, "change", REF_SIGNATURE, exports) != FVM_OK ||
        fvm_styxrpc_export_managed(&second, "reject", REF_SIGNATURE, exports + 1) != FVM_OK ||
        styxrpc_managed_module_init(&module, "flash-refs.dis", exports, 2) != STYXRPC_OK)
    {
        fputs("FlashVM ref export setup failed\n", stderr);
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


/* EN: Create and publish one fixed-size ref Array with own text and byte-array fields.
 * RU: Создать ref-Array фиксированного размера с собственными строкой и массивом. */
static int cell(FVM_Context *context, const char *global, const char *text, FVM_Value *object, FVM_Value *array)
{
    FVM_Value fields[2], byte;
    fvm_value_number(&byte, 7);
    if (fvm_value_string(fields, text) != FVM_OK ||
        fvm_exec_literal_array(context, 1, &byte, fields + 1) != FVM_OK ||
        fvm_set_global(context, "building", fields + 1) != FVM_OK ||
        fvm_exec_literal_array(context, 2, fields, object) != FVM_OK ||
        fvm_set_global(context, global, object) != FVM_OK)
    {
        return 0;
    }
    *array = fields[1];
    return 1;
}


/* EN: Require that both the current field and the unchanged array identity are exact.
 * RU: Сверить строку и тождество неизменённого массива. */
static int cell_is(FVM_Context *context, const FVM_Value *cell, const char *text, const FVM_Value *array)
{
    FVM_Value field;
    return fvm_object_get(context, cell->object, "0", &field) == FVM_OK && field.type == FVM_VALUE_STRING &&
        fvm_value_string_size(&field) == strlen(text) && !memcmp(field.string, text, strlen(text)) &&
        fvm_object_get(context, cell->object, "1", &field) == FVM_OK &&
        field.type == FVM_VALUE_OBJECT && field.object == array->object;
}


/* EN: Call the imported global once with the selected counted command.
 * RU: Вызвать импортированную глобальную функцию с указанной командой. */
static FVM_Result call(FVM_Context *context, FVM_Value *args, const char *command, FVM_Value *result)
{
    return fvm_value_string(args, command) == FVM_OK ?
        fvm_call_bytes(context, "change", 6, 4, args, result) : FVM_INVALID_ARGUMENT;
}


/* EN/RU: See ref-test.h. / См. ref-test.h. */
int test_refs_call(FVM_Context *context, StyxRpcManagedTarget *target)
{
    FVM_Value args[4], first, other, result;
    target->module = "mounted/refs.dis";
    target->name = "change";
    target->signature = REF_SIGNATURE;
    target->schema = schema;
    target->pid = TEST_MANAGED_PID;
    target->wait_ms = REF_WAIT_MS;
    int ok = fvm_styxrpc_import_managed(context, "change", target) == FVM_OK &&
        cell(context, "first", "old", args + 1, &first) &&
        cell(context, "other", "unchanged", args + 3, &other);
    if (ok)
    {
        args[2] = args[1];
        ok = call(context, args, "noop", &result) == FVM_OK &&
            result.type == FVM_VALUE_STRING && !strcmp(result.string, "old") &&
            cell_is(context, args + 1, "old", &first) &&
            call(context, args, "é🌍", &result) == FVM_OK &&
            cell_is(context, args + 2, "é🌍", &first) &&
            cell_is(context, args + 3, "unchanged", &other) &&
            call(context, args, "fail", &result) == FVM_EXCEPTION &&
            fvm_exception_clear(context) == FVM_OK &&
            cell_is(context, args + 1, "é🌍", &first);
    }
    if (ok)
    {
        ok = fvm_object_set_property_flags(context, args[3].object, "0", FVM_PROPERTY_READ_ONLY, 0) == FVM_OK &&
            call(context, args, "both", &result) == FVM_EXCEPTION &&
            fvm_exception_clear(context) == FVM_OK &&
            cell_is(context, args + 1, "é🌍", &first) &&
            cell_is(context, args + 3, "unchanged", &other) &&
            fvm_object_set_property_flags(context, args[3].object, "0", 0, FVM_PROPERTY_READ_ONLY) == FVM_OK &&
            call(context, args, "both", &result) == FVM_OK &&
            cell_is(context, args + 1, "both", &first) && cell_is(context, args + 3, "both", &other);
    }
    if (ok)
    {
        fvm_value_undefined(args + 3);
        args[3].type = FVM_VALUE_NULL;
        ok = call(context, args, "nil", &result) == FVM_OK &&
            cell_is(context, args + 1, "nil", &first);
    }
    if (!ok && fvm_exception_get(context, &result) == FVM_OK && result.type == FVM_VALUE_STRING)
    {
        fprintf(stderr, "FlashVM ref import failed: %s\n", result.string);
    }
    target->session = NULL;
    return ok;
}
