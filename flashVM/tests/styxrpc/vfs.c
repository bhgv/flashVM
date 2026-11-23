/* Copyright (c) 2026 bhgv
 * EN: Check byte-oriented ActionScript VFS methods against the standalone C device.
 * RU: Проверка байтовых методов VFS ActionScript с независимым устройством C.
 */
#define _POSIX_C_SOURCE 200809L
#include "session-test.h"
#include "flashvm/exception.h"
#include "flashvm/execution.h"
#include "flashvm/extensions.h"
#include "flashvm/gc_roots.h"
#include "flashvm/styxrpc_filesystem.h"
#include "flashvm/channel.h"
#include "styxrpc/vfs_client.h"
#include "client.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

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


/* EN: Open the same path through the existing ordinary script file channel method.
 * RU: Открыть тот же путь штатным методом файлового канала сценария. */
static int script_channel(FVM_Context *context, const char *path)
{
    FVM_Value channel, args[3], file, ignored;
    return fvm_extensions_install(context) == FVM_OK &&
        fvm_get_global(context, "channel", &channel) == FVM_OK && channel.type == FVM_VALUE_OBJECT &&
        fvm_value_string_bytes(args, "file", 4) == FVM_OK && fvm_value_string(args + 1, path) == FVM_OK &&
        fvm_value_string(args + 2, "r") == FVM_OK &&
        fvm_object_call(context, channel.object, "create", 3, args, &file) == FVM_OK && file.type == FVM_VALUE_OBJECT &&
        fvm_object_call(context, file.object, "close", 0, NULL, &ignored) == FVM_OK;
}


/* [GC:] EN: Stat, list and open the same mount through ordinary paths, then unmount with a live fid.
 * RU: Выполнить stat, list и open обычных путей, затем снять монтирование с живым fid. */
static int mounted(FVM_Context *context, StyxClient *client)
{
    const char *mount = "/__flashvm_styx__";
    const char *path = "/__flashvm_styx__/mounted/device.dis/value";
    FVM_GCRootScope *roots;
    FVM_Value fs, args[2], file, result, item, byte;
    if (fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return 0;
    }
    int attached = fvm_styxrpc_filesystem_mount(context, mount, client) == FVM_OK;
    FVM_Channel *raw = NULL;
    unsigned char native;
    size_t got;
    int ok = attached && fvm_filesystem_install(context, "fs") == FVM_OK &&
        fvm_get_global(context, "fs", &fs) == FVM_OK && fs.type == FVM_VALUE_OBJECT;
    if (!ok)
    {
        fputs("Could not install mounted filesystem\n", stderr);
    }
    ok = ok &&
        fvm_value_string(args, mount) == FVM_OK &&
        fvm_object_call(context, fs.object, "stat", 1, args, &result) == FVM_OK &&
        result.type == FVM_VALUE_OBJECT && fvm_object_get(context, result.object, "3", &item) == FVM_OK &&
        item.type == FVM_VALUE_BOOLEAN && item.boolean;
    if (ok)
    {
        FVM_Result status = fvm_object_call(context, fs.object, "list", 1, args, &result);
        ok = status == FVM_OK && result.type == FVM_VALUE_OBJECT &&
            fvm_object_get(context, result.object, "0", &item) == FVM_OK && item.type == FVM_VALUE_OBJECT &&
            fvm_object_get(context, item.object, "0", &byte) == FVM_OK && byte.type == FVM_VALUE_STRING &&
            !strcmp(byte.string, "mounted");
        if (!ok)
        {
            fprintf(stderr, "Could not list mounted filesystem root (%d)\n", status);
        }
    }
    if (ok && !(
        fvm_value_string(args, "/__flashvm_styx__/mounted/device.dis/../value") == FVM_OK &&
        fvm_object_call(context, fs.object, "open", 1, args, &result) == FVM_NOT_FOUND &&
        fvm_value_string(args, path) == FVM_OK && fvm_value_string(args + 1, "rwo") == FVM_OK &&
        fvm_object_call(context, fs.object, "open", 2, args, &file) == FVM_OK && file.type == FVM_VALUE_OBJECT &&
        fvm_gc_roots_add(roots, &file) == FVM_OK && script_channel(context, path) &&
        fvm_channel_open_file_in(context, path, "r", &raw) == FVM_OK &&
        fvm_channel_file_read(raw, &native, 1, &got) == FVM_OK && got == 1 && native == 0))
    {
        fputs("Could not open mounted file through ordinary paths\n", stderr);
        ok = 0;
    }
    FVM_Result removed = attached ? fvm_filesystem_unmount(context, mount) : FVM_ERROR;
    fvm_channel_release(raw);
    if (ok && removed == FVM_OK)
    {
        fvm_value_number(&byte, 255);
        ok = fvm_exec_literal_array(context, 1, &byte, args) == FVM_OK &&
            fvm_object_call(context, file.object, "write", 1, args, &result) == FVM_OK &&
            result.type == FVM_VALUE_NUMBER && result.number == 1;
        fvm_value_number(args, 0);
        ok = ok && fvm_object_call(context, file.object, "seek", 1, args, &result) == FVM_OK &&
            result.type == FVM_VALUE_NUMBER && result.number == 0;
        fvm_value_number(args, 2);
        ok = ok && fvm_object_call(context, file.object, "read", 1, args, &result) == FVM_OK &&
            result.type == FVM_VALUE_OBJECT && fvm_object_get(context, result.object, "length", &item) == FVM_OK &&
            item.type == FVM_VALUE_NUMBER && item.number == 1 &&
            fvm_object_get(context, result.object, "0", &item) == FVM_OK && item.type == FVM_VALUE_NUMBER &&
            item.number == 255 && fvm_object_call(context, file.object, "close", 0, NULL, &result) == FVM_OK;
    }
    else
    {
        ok = 0;
    }
    fvm_gc_roots_end(roots);
    return ok;
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
        exercise(context, object.object) && mounted(context, client);
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


/* EN: Wait until the Limbo owner changes the live cell, then update it through the property.
 * RU: Дождаться изменения ячейки владельцем Limbo и обновить её через свойство. */
static int replace_cell(FVM_Context *context, FVM_Object *scope)
{
    const struct timespec pause = {0, 10000000};
    for (unsigned attempt = 0; attempt < 300; ++attempt)
    {
        FVM_Value value;
        if (fvm_object_get(context, scope, "count", &value) != FVM_OK || value.type != FVM_VALUE_NUMBER)
        {
            return 0;
        }
        if (value.number == -2)
        {
            fvm_value_number(&value, 65539);
            return fvm_object_set(context, scope, "count", &value) == FVM_OK;
        }
        nanosleep(&pause, NULL);
    }
    return 0;
}


/* EN: Inspect and open the actual native .var tree through ordinary FlashVM paths.
 * RU: Просмотреть и открыть настоящее дерево .var через обычные пути FlashVM. */
static int native_filesystem(FVM_Context *context, StyxClient *client, const char *pid)
{
    const char *mount = "/__dis__";
    FVM_Value fs, arg, result, item;
    FVM_Channel *file = NULL;
    char directory[96], path[104];
    int chars = snprintf(directory, sizeof(directory), "%s/loaded.dis/.prog/%s/.var", mount, pid);
    if (chars < 0 || (size_t)chars >= sizeof(directory) || fvm_styxrpc_filesystem_mount(context, mount, client) != FVM_OK)
    {
        return 0;
    }
    snprintf(path, sizeof(path), "%s/limit", directory);
    unsigned char bytes[4];
    const unsigned char expected[] = {0, 0, 0, 73};
    size_t size;
    int ok = fvm_filesystem_install(context, "fs") == FVM_OK &&
        fvm_get_global(context, "fs", &fs) == FVM_OK && fs.type == FVM_VALUE_OBJECT &&
        fvm_value_string(&arg, directory) == FVM_OK &&
        fvm_object_call(context, fs.object, "list", 1, &arg, &result) == FVM_OK && result.type == FVM_VALUE_OBJECT &&
        fvm_object_get(context, result.object, "length", &item) == FVM_OK && item.type == FVM_VALUE_NUMBER && item.number == 2 &&
        fvm_channel_open_file_in(context, path, "r", &file) == FVM_OK &&
        fvm_channel_file_read(file, bytes, sizeof(bytes), &size) == FVM_OK && size == sizeof(bytes) &&
        !memcmp(bytes, expected, sizeof(bytes));
    fvm_channel_release(file);
    return fvm_filesystem_unmount(context, mount) == FVM_OK && ok;
}


/* EN/RU: See session-test.h. / См. session-test.h. */
int test_variable_call(FVM_Context *context, int fd, const char *pid)
{
    StyxClient *client = styxclientmountfd(fd, DEVICE_MSIZE, "flash", "", DEVICE_WAIT_MS, DEVICE_TAGS);
    FVM_Value owner, value;
    char path[96];
    int size = snprintf(path, sizeof(path), "loaded.dis/.prog/%s/.var/limit", pid);
    fvm_value_undefined(&owner);
    int ok = client && size > 0 && (size_t)size < sizeof(path) && native_filesystem(context, client, pid) &&
        fvm_object_create(context, &owner.object) == FVM_OK;
    if (ok)
    {
        /* Context owns the scope through both accessors and pins their exact fids. */
        owner.type = FVM_VALUE_OBJECT;
        ok = fvm_set_global(context, "external", &owner) == FVM_OK &&
            fvm_styxrpc_int_variable_bind(context, owner.object, "limit", client, path, 0) == FVM_OK &&
            fvm_object_get(context, owner.object, "limit", &value) == FVM_OK &&
            value.type == FVM_VALUE_NUMBER && value.number == 73;
    }
    if (ok)
    {
        fvm_value_number(&value, 999);
        size = snprintf(path, sizeof(path), "loaded.dis/.prog/%s/.var/count", pid);
        ok = fvm_object_set(context, owner.object, "limit", &value) == FVM_EXCEPTION &&
            fvm_exception_clear(context) == FVM_OK && size > 0 && (size_t)size < sizeof(path) &&
            fvm_styxrpc_int_variable_bind(context, owner.object, "count", client, path, 1) == FVM_OK &&
            replace_cell(context, owner.object);
    }
    styxclientshutdown(client);
    styxclientfree(client);
    return ok;
}
