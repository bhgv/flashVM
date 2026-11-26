/* Copyright (c) 2026 bhgv
 * EN: Positional read, write and stat against an attached Styx VFS from AS.
 * RU: Позиционные чтение, запись и stat смонтированной VFS Styx из AS.
 */
#include "session_methods.h"
#include "flashvm/execution.h"
#include "styxrpc/vfs_client.h"
#include <limits.h>
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
    char path[FVM_STYXRPC_ABSOLUTE_PATH];
    unsigned char bytes[FVM_STYXRPC_SCRIPT_BYTES];
    uint64_t offset, requested;
    size_t used, i;
    FVM_Value *items;
    FVM_Value data;
    FVM_Result materialized;
    StyxRpcIoOutcome status;
    (void)self;
    if (argc != 3 || !fvm_styxrpc_resolve_path(binding, args, path) ||
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
    status = styxrpc_vfs_read_at(binding->client, path + 1, offset, bytes, (size_t)requested, &used);
    for (i = 0; i < used; ++i)
    {
        fvm_value_number(items + i, bytes[i]);
    }
    materialized = fvm_exec_literal_array(context, (unsigned)used, items, &data);
    free(items);
    return materialized == FVM_OK ? pair(context, status, &data, result) : materialized;
}


/* EN: Read length once and validate its exact byte extent before copying.
 * RU: Один раз прочитать длину и проверить точный размер в байтах. */
static FVM_Result byte_extent(FVM_Context *context, const FVM_Value *value, size_t capacity, size_t *size)
{
    FVM_Value property;
    uint64_t count;
    FVM_Result result;
    if (value->type != FVM_VALUE_OBJECT || !value->object)
    {
        return FVM_INVALID_ARGUMENT;
    }
    result = fvm_object_get(context, value->object, "length", &property);
    if (result != FVM_OK)
    {
        return result;
    }
    if (!fvm_styxrpc_number(&property, capacity < UINT_MAX ? capacity : UINT_MAX, &count))
    {
        return FVM_INVALID_ARGUMENT;
    }
    *size = (size_t)count;
    return FVM_OK;
}


/* EN: Copy count exact own indexed bytes in order. Stop at the first invalid slot.
 * RU: Скопировать count точных собственных байтов по порядку до первого неверного слота. */
static FVM_Result copy_bytes(FVM_Context *context, FVM_Object *object, unsigned char *bytes, size_t count)
{
    unsigned i;
    for (i = 0; i < count; ++i)
    {
        char name[FVM_STYXRPC_INDEX_BYTES];
        FVM_Value property;
        uint64_t byte;
        int own;
        FVM_Result result;
        sprintf(name, "%u", i);
        result = fvm_object_has_own(context, object, name, &own);
        if (result != FVM_OK || !own)
        {
            return result != FVM_OK ? result : FVM_INVALID_ARGUMENT;
        }
        result = fvm_object_get(context, object, name, &property);
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
    return FVM_OK;
}


/* EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_bytes(FVM_Context *context, const FVM_Value *value, unsigned char *bytes,
    size_t capacity, size_t *size)
{
    size_t count;
    FVM_Result status = byte_extent(context, value, capacity, &count);
    if (status == FVM_OK)
    {
        status = copy_bytes(context, value->object, bytes, count);
    }
    if (status == FVM_OK)
    {
        *size = count;
    }
    return status;
}


/* EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_alloc_bytes(FVM_Context *context, const FVM_Value *value,
    size_t limit, unsigned char **bytes, size_t *size)
{
    size_t count;
    unsigned char *snapshot;
    FVM_Result status = byte_extent(context, value, limit, &count);
    if (status != FVM_OK)
    {
        return status;
    }
    snapshot = count ? malloc(count) : NULL;
    if (count && !snapshot)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = copy_bytes(context, value->object, snapshot, count);
    if (status != FVM_OK)
    {
        free(snapshot);
        return status;
    }
    *bytes = snapshot;
    *size = count;
    return FVM_OK;
}


/* EN: Write a validated snapshot, returning status and the confirmed byte count.
 * RU: Записать проверенный снимок и вернуть статус и подтверждённое число байтов. */
static FVM_Result write_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    const FVM_StyxRpcSession *binding = state;
    char path[FVM_STYXRPC_ABSOLUTE_PATH];
    unsigned char bytes[FVM_STYXRPC_SCRIPT_BYTES];
    uint64_t offset;
    size_t size, used;
    FVM_Value data;
    FVM_Result status;
    StyxRpcIoOutcome written;
    (void)self;
    if (argc != 3 || !fvm_styxrpc_resolve_path(binding, args, path) ||
        !fvm_styxrpc_number(args + 1, FVM_STYXRPC_EXACT_OFFSET, &offset))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_styxrpc_bytes(context, args + 2, bytes, sizeof(bytes), &size);
    if (status != FVM_OK)
    {
        return status;
    }
    written = styxrpc_vfs_write_at(binding->client, path + 1, offset, bytes, size, &used);
    fvm_value_number(&data, (double)used);
    return pair(context, written, &data, result);
}


/* EN: Read metadata and return the length as two exact unsigned limbs.
 * RU: Прочитать метаданные и вернуть длину двумя точными беззнаковыми частями. */
static FVM_Result stat_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    const FVM_StyxRpcSession *binding = state;
    char path[FVM_STYXRPC_ABSOLUTE_PATH];
    StyxRpcFileInfo info = {0, 0, 0};
    FVM_Value fields[5];
    StyxRpcIoOutcome status = STYXRPC_IO_NOT_OPEN;
    FVM_StyxRpcPathResult path_result = argc == 1 ? fvm_styxrpc_path_result(binding, args, path) :
        FVM_STYXRPC_PATH_INVALID;
    (void)self;
    if (path_result == FVM_STYXRPC_PATH_INVALID)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (path_result == FVM_STYXRPC_PATH_VISIBLE)
    {
        status = styxrpc_vfs_stat(binding->client, path + 1, &info);
    }
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
