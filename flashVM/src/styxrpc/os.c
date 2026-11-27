/* Copyright (c) 2026 bhgv
 * EN: ActionScript OS objects backed by exact file and directory fids on the player session.
 * RU: Объекты OS ActionScript с точными fid файлов и каталогов сессии плеера.
 */
#include "os_handles.h"
#include "session_methods.h"
#include "managed_values.h"
#include "module_import.h"
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/styxrpc_module.h"
#include "flashvm/userdata.h"
#include <stdlib.h>
#include <string.h>

#define EXACT_OFFSET UINT64_C(9007199254740991) /* Largest uniformly exact AS offset. */

typedef struct OsEntry
{
    FVM_Context *context; /* Owner creating the result. */
    FVM_Value *out;       /* Receives the entry. */
    FVM_Result status;    /* Allocation/formatting status of the visitor. */
} OsEntry;


/* EN: Clunk all held fids now or defer the wire operations in GC.
 * RU: Закрыть все удерживаемые fid сразу либо отложить обмен при GC. */
static void retire(OsHandle *handle, int deferred)
{
    styxrpc_vfs_close(handle->file, deferred);
    styxrpc_vfs_dir_close(handle->directory, deferred);
    styxrpc_vfs_close(handle->module, deferred);
    handle->file = NULL;
    handle->directory = NULL;
    handle->module = NULL;
}


/* EN: Release native storage without dereferencing the playback context.
 * RU: Освободить нативную память без обращения к контексту воспроизведения. */
static void release(void *state)
{
    OsHandle *handle = state;
    retire(handle, 1);
    free(handle);
}


/* [ASINC] EN: Explicit idempotent close on the movie owner.
 * RU: Явное идемпотентное закрытие у владельца фильма. */
static FVM_Result close_handle(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    (void)context;
    (void)self;
    (void)args;
    if (argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    retire(state, 0);
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


/* [ASINC] [GC:] EN: Return the confirmed bytes at the current cursor as an AS Array.
 * RU: Вернуть подтверждённые байты с курсора в виде массива AS. */
static FVM_Result read_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    OsHandle *handle = state;
    unsigned char bytes[FVM_STYXRPC_SCRIPT_BYTES];
    FVM_Value *values;
    uint64_t count;
    size_t used, i;
    FVM_Result status;
    (void)self;
    if (!handle->file || argc != 1 || !fvm_styxrpc_number(args, sizeof(bytes), &count) ||
        count > EXACT_OFFSET - handle->offset)
    {
        return FVM_INVALID_ARGUMENT;
    }
    values = count ? malloc((size_t)count * sizeof(*values)) : NULL;
    if (count && !values)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (styxrpc_vfs_file_read(handle->file, handle->offset, bytes, (size_t)count, &used) != STYXRPC_IO_OK)
    {
        handle->offset += used;
        free(values);
        return fvm_styxrpc_managed_fail(context, "os file read failed");
    }
    handle->offset += used;
    for (i = 0; i < used; ++i)
    {
        fvm_value_number(values + i, bytes[i]);
    }
    status = fvm_exec_literal_array(context, (unsigned)used, values, out);
    free(values);
    return status;
}


/* [ASINC] EN: Write a byte-array snapshot and return the confirmed count.
 * RU: Записать снимок массива байтов и вернуть подтверждённое число. */
static FVM_Result write_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    OsHandle *handle = state;
    unsigned char bytes[FVM_STYXRPC_SCRIPT_BYTES];
    size_t size, used;
    FVM_Result checked;
    StyxRpcIoOutcome status;
    (void)self;
    if (!handle->file || handle->uncertain || argc != 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    checked = fvm_styxrpc_bytes(context, args, bytes, sizeof(bytes), &size);
    if (checked != FVM_OK)
    {
        return checked;
    }
    if (!handle->file || handle->uncertain || size > EXACT_OFFSET - handle->offset)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = styxrpc_vfs_file_write(handle->file, handle->offset, bytes, size, &used);
    handle->offset += used;
    if (status != STYXRPC_IO_OK)
    {
        handle->uncertain = status == STYXRPC_IO_UNCERTAIN;
        return fvm_styxrpc_managed_fail(context, handle->uncertain ?
            "os file write outcome is uncertain" : "os file write failed");
    }
    fvm_value_number(out, (double)used);
    return FVM_OK;
}


/* EN: Report client write acknowledgement state. This client buffers no writes.
 * RU: Вернуть состояние подтверждения записи. Клиент не буферизует записи. */
static FVM_Result flush_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    OsHandle *handle = state;
    (void)context;
    (void)self;
    (void)args;
    if (!handle->file || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(out, !handle->uncertain);
    return FVM_OK;
}


/* [ASINC] EN: Obtain the live byte length on the exact file.
 * RU: Получить текущую длину в байтах у точного файла. */
static FVM_Result file_length(FVM_Context *context, OsHandle *handle, double *size)
{
    StyxRpcFileInfo info;
    if (styxrpc_vfs_file_stat(handle->file, &info) != STYXRPC_IO_OK || info.length > EXACT_OFFSET)
    {
        return fvm_styxrpc_managed_fail(context, "os file length unavailable or out of range");
    }
    *size = (double)info.length;
    return FVM_OK;
}


/* [ASINC] EN: Return the live byte length.
 * RU: Вернуть текущую длину в байтах. */
static FVM_Result len_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_Result status;
    double size;
    (void)self;
    (void)args;
    if (argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = file_length(context, state, &size);
    if (status == FVM_OK)
    {
        fvm_value_number(out, size);
    }
    return status;
}


/* [ASINC] EN: Move the cursor from start, current position or live EOF.
 * RU: Переместить курсор от начала, текущей позиции или текущего EOF. */
static FVM_Result seek_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    OsHandle *handle = state;
    char origin[4] = "set";
    double base = 0;
    uint64_t offset;
    FVM_Value position;
    (void)self;
    if (!handle->file || !argc || argc > 2 || args[0].type != FVM_VALUE_NUMBER ||
        (argc == 2 && !fvm_styxrpc_name(args + 1, origin, sizeof(origin))))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!strcmp(origin, "cur"))
    {
        base = (double)handle->offset;
    }
    else if (!strcmp(origin, "end"))
    {
        FVM_Result status = file_length(context, handle, &base);
        if (status != FVM_OK)
        {
            return status;
        }
    }
    else if (strcmp(origin, "set"))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!(args[0].number >= -(double)EXACT_OFFSET && args[0].number <= (double)EXACT_OFFSET) ||
        (double)(int64_t)args[0].number != args[0].number)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_number(&position, base + args[0].number);
    if (!fvm_styxrpc_number(&position, EXACT_OFFSET, &offset))
    {
        return FVM_INVALID_ARGUMENT;
    }
    handle->offset = offset;
    *out = position;
    return FVM_OK;
}


/* EN: Match the literal extension at the end of a canonical path.
 * RU: Сопоставить буквальное расширение в конце канонического пути. */
int fvm_os_extension(const char *path, const char *suffix)
{
    size_t size = strlen(path);
    return size >= 4 && !strcmp(path + size - 4, suffix);
}


/* [ASINC] [GC:] EN: Load a SWF into the movie's display tree or materialize a checked module proxy.
 * RU: Загрузить SWF в дерево фильма либо создать проверенный прокси модуля. */
static FVM_Result load_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    OsHandle *handle = state;
    struct StyxFid *live = NULL, *lease = NULL, *module;
    (void)self;
    (void)args;
    if (argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!handle->module && handle->file && fvm_os_extension(handle->path, ".dis"))
    {
        live = styxrpc_module_live_open(handle->movies->session, handle->path + 1, &lease);
        if (!live)
        {
            return FVM_NOT_FOUND;
        }
    }
    module = live ? live : handle->module;
    if (module)
    {
        StyxRpcModuleExports exports;
        FVM_StyxRpcLoadSpec spec = {0};
        FVM_Result status = FVM_NOT_FOUND;
        spec.session = handle->movies->session;
        spec.path = handle->path + 1;
        spec.pid = handle->movies->binding->program.pid;
        spec.bus = handle->movies->argument_bus;
        if (styxrpc_module_exports_at(module, spec.pid, &exports) == STYXRPC_OK)
        {
            spec.methods = exports.methods;
            spec.count = exports.count;
            spec.wait_ms = FVM_SESSION_CALL_WAIT_MS;
            status = fvm_styxrpc_module_load_at(context, module, &spec, &lease, out);
        }
        styxrpc_vfs_close(live, 0);
        styxrpc_vfs_close(lease, 0);
        return status;
    }
    if (handle->movies->root && handle->file && fvm_os_extension(handle->path, ".swf"))
    {
        unsigned char *bytes;
        size_t size;
        FVM_Object *clip;
        FVM_Result status = fvm_styxrpc_read_open_asset(handle->file, handle->movies->limit, &bytes, &size);
        if (status == FVM_OK)
        {
            status = fvm_movieclip_load_child_swf(context, handle->movies->root, bytes, size, handle->path, &clip);
            free(bytes);
        }
        if (status == FVM_OK)
        {
            fvm_value_undefined(out);
            out->type = FVM_VALUE_OBJECT;
            out->object = clip;
        }
        return status;
    }
    return FVM_UNSUPPORTED;
}


/* [GC:] EN: Create a fresh attributes object for a VFS entry.
 * RU: Создать объект атрибутов записи VFS. */
FVM_Result fvm_os_attributes(FVM_Context *context, const StyxRpcFileInfo *info, const char *name, FVM_Value *out)
{
    const char *keys[] = {"length", "lengthHigh", "lengthLow", "mode", "directory", "name"};
    FVM_Value values[6], object;
    FVM_GCRootScope *roots;
    unsigned i;
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_value_undefined(&object);
    status = fvm_object_create(context, &object.object);
    if (status == FVM_OK)
    {
        object.type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, &object);
    }
    fvm_value_undefined(values);
    if (info->length <= EXACT_OFFSET)
    {
        fvm_value_number(values, (double)info->length);
    }
    fvm_value_number(values + 1, (double)(info->length >> 32));
    fvm_value_number(values + 2, (double)(uint32_t)info->length);
    fvm_value_number(values + 3, info->mode);
    fvm_value_boolean(values + 4, info->directory);
    if (status == FVM_OK)
    {
        status = fvm_value_string(values + 5, name);
    }
    for (i = 0; i < 6 && status == FVM_OK; ++i)
    {
        status = fvm_object_set(context, object.object, keys[i], values + i);
    }
    if (status == FVM_OK)
    {
        *out = object;
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* [GC:] EN: Copy one borrowed directory entry to the result.
 * RU: Скопировать одну заимствованную запись каталога в результат. */
static int entry(void *state, const char *name, const StyxRpcFileInfo *info)
{
    OsEntry *item = state;
    item->status = fvm_os_attributes(item->context, info, name, item->out);
    return item->status == FVM_OK;
}


/* [ASINC] [GC:] EN: Return the next visible entry or undefined at EOF.
 * RU: Вернуть следующую видимую запись либо undefined в конце каталога. */
static FVM_Result next_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    OsHandle *handle = state;
    OsEntry item;
    int found;
    (void)self;
    (void)args;
    if (!handle->directory || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    item.context = context;
    item.out = out;
    item.status = FVM_OK;
    fvm_value_undefined(out);
    if (styxrpc_vfs_dir_next(handle->directory, entry, &item, &found) != STYXRPC_IO_OK)
    {
        return item.status == FVM_OK ? fvm_styxrpc_managed_fail(context, "os directory read failed") : item.status;
    }
    return FVM_OK;
}


/* EN: Discard buffered entries so the next read starts at zero on this fid.
 * RU: Сбросить буфер записей. Следующее чтение начнётся с нуля на том же fid. */
static FVM_Result rewind_directory(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    OsHandle *handle = state;
    (void)context;
    (void)self;
    (void)args;
    if (argc || styxrpc_vfs_dir_rewind(handle->directory) != STYXRPC_IO_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


/* [GC:] EN: Install the selected handle methods, retaining the object through extracted methods.
 * RU: Установить методы дескриптора с удержанием объекта извлечёнными методами. */
FVM_Result fvm_os_wrap_handle(FVM_Context *context, OsHandle *handle, int directory, FVM_Value *out)
{
    static const char *const names[] = {"close", "read", "write", "flush", "seek", "len", "load", "next_file", "rewind"};
    static const FVM_NativeFunction calls[] =
        {close_handle, read_file, write_file, flush_file, seek_file, len_file, load_file, next_file, rewind_directory};
    FVM_GCRootScope *roots;
    FVM_Value value;
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    unsigned i;
    int owned = 0;
    if (status != FVM_OK)
    {
        retire(handle, 0);
        free(handle);
        return status;
    }
    fvm_value_undefined(&value);
    status = fvm_object_create(context, &value.object);
    if (status == FVM_OK)
    {
        value.type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, &value);
    }
    if (status == FVM_OK)
    {
        status = fvm_userdata_attach(context, value.object, handle, release);
        owned = status == FVM_OK;
    }
    for (i = 0; i < sizeof(calls) / sizeof(calls[0]) && status == FVM_OK; ++i)
    {
        if ((!i || (directory ? i >= 7 : i < 7)) &&
            (calls[i] != load_file || handle->module || handle->movies->root ||
                (handle->file && fvm_os_extension(handle->path, ".dis"))))
        {
            status = fvm_object_define_method_rooted(context, value.object, names[i], calls[i], handle, value.object);
        }
    }
    if (status == FVM_OK)
    {
        *out = value;
    }
    else
    {
        retire(handle, 0);
        if (!owned)
        {
            free(handle);
        }
    }
    fvm_gc_roots_end(roots);
    return status;
}
