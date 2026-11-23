/* Copyright (c) 2026 bhgv
 * EN: Positional read, write and stat against an attached Styx VFS from AS.
 * RU: Позиционные чтение, запись и stat смонтированной VFS Styx из AS.
 */
#include "session_methods.h"
#include "flashvm/execution.h"
#include "styxrpc/vfs_client.h"
#include <stdio.h>
#include <stdlib.h>

#define FVM_STYXRPC_EXACT_OFFSET UINT64_C(9007199254740991) /* Largest uniformly exact AS integer. */
#define FVM_STYXRPC_INDEX_BYTES 11U /* Decimal uint32 index including terminating zero. */
#define FVM_STYXRPC_U32_BITS 32U /* Limb width of the AS length result. */


/* EN: Return a status and a scalar or Array as a single AS result.
 * RU: Вернуть статус и число либо массив как один результат AS. */
static FVM_Result pair(FVM_Context *context, StyxRpcIoOutcome status, const FVM_Value *data, FVM_Value *result)
{
    FVM_Value fields[2];
    fvm_value_number(fields, status);
    fields[1] = *data;
    return fvm_exec_literal_array(context, 2, fields, result);
}


/* [GC:] EN: Read a bounded prefix and materialize binary bytes as an AS Array.
 * RU: Прочитать ограниченный префикс и представить двоичные байты как массив AS. */
static FVM_Result read_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    const FVM_StyxRpcSession *binding = state;
    char path[STYXRPC_VFS_PATH_MAX + 1];
    unsigned char bytes[FVM_STYXRPC_SCRIPT_BYTES];
    uint64_t offset, requested;
    size_t used, i;
    FVM_Value *items;
    FVM_Value data;
    FVM_Result materialized;
    StyxRpcIoOutcome status;
    (void)self;
    if (argc != 3 || !fvm_styxrpc_name(args, path, sizeof(path)) ||
        !fvm_styxrpc_number(args + 1, FVM_STYXRPC_EXACT_OFFSET, &offset) ||
        !fvm_styxrpc_number(args + 2, FVM_STYXRPC_SCRIPT_BYTES, &requested))
    {
        return FVM_INVALID_ARGUMENT;
    }
    items = requested ? malloc((size_t)requested * sizeof(*items)) : NULL;
    if (requested && !items)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = styxrpc_vfs_read_at(binding->client, path, offset, bytes, (size_t)requested, &used);
    for (i = 0; i < used; ++i)
    {
        fvm_value_number(items + i, bytes[i]);
    }
    materialized = fvm_exec_literal_array(context, (unsigned)used, items, &data);
    free(items);
    return materialized == FVM_OK ? pair(context, status, &data, result) : materialized;
}


/* EN: Snapshot exact own byte properties before any native write.
 * RU: Снять точные собственные байтовые свойства до нативной записи. */
static FVM_Result byte_array(FVM_Context *context, const FVM_Value *value, unsigned char *bytes, size_t *size)
{
    FVM_Value property;
    uint64_t count, byte;
    unsigned i;
    FVM_Result result;
    int own;
    if (value->type != FVM_VALUE_OBJECT)
    {
        return FVM_INVALID_ARGUMENT;
    }
    result = fvm_object_get(context, value->object, "length", &property);
    if (result != FVM_OK)
    {
        return result;
    }
    if (!fvm_styxrpc_number(&property, FVM_STYXRPC_SCRIPT_BYTES, &count))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (i = 0; i < (unsigned)count; ++i)
    {
        char name[FVM_STYXRPC_INDEX_BYTES];
        sprintf(name, "%u", i);
        result = fvm_object_has_own(context, value->object, name, &own);
        if (result != FVM_OK || !own)
        {
            return result != FVM_OK ? result : FVM_INVALID_ARGUMENT;
        }
        result = fvm_object_get(context, value->object, name, &property);
        if (result != FVM_OK)
        {
            return result;
        }
        if (!fvm_styxrpc_number(&property, UINT8_MAX, &byte))
        {
            return FVM_INVALID_ARGUMENT;
        }
        bytes[i] = (unsigned char)byte;
    }
    *size = (size_t)count;
    return FVM_OK;
}


/* EN: Write a validated snapshot, returning status and the confirmed byte count.
 * RU: Записать проверенный снимок и вернуть статус и подтверждённое число байтов. */
static FVM_Result write_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    const FVM_StyxRpcSession *binding = state;
    char path[STYXRPC_VFS_PATH_MAX + 1];
    unsigned char bytes[FVM_STYXRPC_SCRIPT_BYTES];
    uint64_t offset;
    size_t size, used;
    FVM_Value data;
    FVM_Result status;
    StyxRpcIoOutcome written;
    (void)self;
    if (argc != 3 || !fvm_styxrpc_name(args, path, sizeof(path)) ||
        !fvm_styxrpc_number(args + 1, FVM_STYXRPC_EXACT_OFFSET, &offset))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = byte_array(context, args + 2, bytes, &size);
    if (status != FVM_OK)
    {
        return status;
    }
    written = styxrpc_vfs_write_at(binding->client, path, offset, bytes, size, &used);
    fvm_value_number(&data, (double)used);
    return pair(context, written, &data, result);
}


/* EN: Read metadata and return the length as two exact unsigned limbs.
 * RU: Прочитать метаданные и вернуть длину двумя точными беззнаковыми частями. */
static FVM_Result stat_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    const FVM_StyxRpcSession *binding = state;
    char path[STYXRPC_VFS_PATH_MAX + 1];
    StyxRpcFileInfo info = {0, 0, 0};
    FVM_Value fields[5];
    StyxRpcIoOutcome status;
    (void)self;
    if (argc != 1 || !fvm_styxrpc_name(args, path, sizeof(path)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = styxrpc_vfs_stat(binding->client, path, &info);
    fvm_value_number(fields, status);
    fvm_value_number(fields + 1, (double)(info.length >> FVM_STYXRPC_U32_BITS));
    fvm_value_number(fields + 2, (double)(uint32_t)info.length);
    fvm_value_number(fields + 3, (double)info.mode);
    fvm_value_boolean(fields + 4, info.directory);
    return fvm_exec_literal_array(context, 5, fields, result);
}


/* EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_vfs_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding)
{
    FVM_Result result = fvm_object_define_method_rooted(context, object, "read", read_file, binding, NULL);
    if (result == FVM_OK)
    {
        result = fvm_object_define_method_rooted(context, object, "write", write_file, binding, NULL);
    }
    if (result == FVM_OK)
    {
        result = fvm_object_define_method_rooted(context, object, "stat", stat_file, binding, NULL);
    }
    return result;
}
