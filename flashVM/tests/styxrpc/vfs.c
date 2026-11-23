/* Copyright (c) 2026 bhgv
 * EN: Check byte-oriented ActionScript VFS methods against the standalone C device.
 * RU: Проверка байтовых методов VFS ActionScript с независимым устройством C.
 */
#include "session-test.h"
#include "flashvm/execution.h"
#include "styxrpc/vfs_client.h"
#include "client.h"
#include <stdio.h>

#define DEVICE_MSIZE 4096U   /* Requested native Styx message bound. */
#define DEVICE_WAIT_MS 3000U /* I/O deadlines on the delegated connection. */
#define DEVICE_TAGS 8U       /* Native request slots. */


/* EN: Read one status/data pair, checking the exact single byte when requested.
 * RU: Прочитать пару статуса и данных, при необходимости сверив один байт. */
static int read_pair(FVM_Context *context, FVM_Object *object, FVM_Value *args,
    StyxRpcIoOutcome expected, int byte)
{
    FVM_Value result, status, data, item;
    return fvm_object_call(context, object, "read", 3, args, &result) == FVM_OK &&
        result.type == FVM_VALUE_OBJECT && fvm_object_get(context, result.object, "0", &status) == FVM_OK &&
        status.type == FVM_VALUE_NUMBER && status.number == expected &&
        fvm_object_get(context, result.object, "1", &data) == FVM_OK && data.type == FVM_VALUE_OBJECT &&
        fvm_object_get(context, data.object, "length", &item) == FVM_OK && item.type == FVM_VALUE_NUMBER &&
        item.number == (byte < 0 ? 0 : 1) &&
        (byte < 0 || (fvm_object_get(context, data.object, "0", &item) == FVM_OK &&
            item.type == FVM_VALUE_NUMBER && item.number == byte));
}


/* EN: Verify partial writes, a returned NUL byte, EOF and refusal of unsafe paths.
 * RU: Проверить частичную запись, нулевой байт, EOF и отклонение запрещённых путей. */
static int exercise(FVM_Context *context, FVM_Object *object)
{
    FVM_Value args[3], bytes[2], result, status, count;
    if (fvm_value_string(args, "mounted/device.dis/value") != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(args + 1, 0);
    fvm_value_number(args + 2, 3);
    if (!read_pair(context, object, args, STYXRPC_IO_OK, 'A'))
    {
        return 0;
    }
    fvm_value_number(bytes, 0);
    fvm_value_number(bytes + 1, 255);
    if (fvm_exec_literal_array(context, 2, bytes, args + 2) != FVM_OK ||
        fvm_object_call(context, object, "write", 3, args, &result) != FVM_OK ||
        result.type != FVM_VALUE_OBJECT || fvm_object_get(context, result.object, "0", &status) != FVM_OK ||
        status.type != FVM_VALUE_NUMBER || status.number != STYXRPC_IO_OK ||
        fvm_object_get(context, result.object, "1", &count) != FVM_OK ||
        count.type != FVM_VALUE_NUMBER || count.number != 1)
    {
        return 0;
    }
    fvm_value_number(bytes, 256);
    if (fvm_exec_literal_array(context, 2, bytes, args + 2) != FVM_OK ||
        fvm_object_call(context, object, "write", 3, args, &result) != FVM_INVALID_ARGUMENT)
    {
        return 0;
    }
    fvm_value_number(args + 2, 3);
    if (!read_pair(context, object, args, STYXRPC_IO_OK, 0))
    {
        return 0;
    }
    fvm_value_number(args + 1, 1);
    return read_pair(context, object, args, STYXRPC_IO_OK, -1) &&
        fvm_value_string(args, "mounted/device.dis/../value") == FVM_OK &&
        read_pair(context, object, args, STYXRPC_IO_NOT_OPEN, -1);
}


/* EN/RU: See session-test.h. / См. session-test.h. */
int test_vfs_call(FVM_Context *context, FVM_StyxRpcSession *binding, int fd)
{
    StyxClient *client = styxclientmountfd(fd, DEVICE_MSIZE, "flash", "", DEVICE_WAIT_MS, DEVICE_TAGS);
    FVM_Value object, args[3], refused;
    int ok = client && fvm_styxrpc_session_bind(context, binding, client, NULL, &object) == FVM_OK &&
        fvm_set_global(context, "vfs", &object) == FVM_OK &&
        fvm_styxrpc_session_bind(context, binding, client, NULL, &refused) == FVM_INVALID_ARGUMENT &&
        fvm_value_string(args, "context.dis") == FVM_OK &&
        fvm_value_string(args + 1, "") == FVM_OK &&
        fvm_object_call(context, object.object, "launch", 2, args, &refused) == FVM_OK &&
        refused.type == FVM_VALUE_NUMBER && refused.number == STYXRPC_LAUNCH_NOT_SENT &&
        exercise(context, object.object);
    fvm_styxrpc_session_close(binding);
    if (ok)
    {
        fvm_value_string(args, "mounted/device.dis/value");
        fvm_value_number(args + 1, 0);
        fvm_value_number(args + 2, 1);
        ok = read_pair(context, object.object, args, STYXRPC_IO_NOT_OPEN, -1);
    }
    styxclientshutdown(client);
    styxclientfree(client);
    return ok;
}
