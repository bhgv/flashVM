/* Copyright (c) 2026 bhgv
 * EN: Native lifetime for an exact request/reply grant.
 * RU: Нативное владение точными файлами запроса и ответа.
 */
#include "scalar_files.h"
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "../../native/terminal/scalar_record.h"
#include "../../native/terminal/scalar_verify.h"
#include <limits.h>
#include <stdlib.h>


/* EN/RU: Consume route storage and its two file owners.
 * Освободить маршрут и обе его ссылки на файлы. */
static void destroy_files(void *state)
{
    DVM_ScalarFiles *files = state;
    dvm_provider_release(files->request);
    dvm_provider_release(files->reply);
    free(files);
}

static const DVM_VfsOps cleanup = {.close = destroy_files}; /* Native closer callback. */


/* EN/RU: Consume captured file grants and reserve final close before publishing.
 * Принять захваченные файлы и зарезервировать закрытие до публикации. */
static DVM_ScalarFiles *new_files(DVM_Provider *request, DVM_Provider *reply, int duplex)
{
    DVM_ScalarFiles *files = calloc(1, sizeof(*files));
    if (!files)
    {
        dvm_provider_release(request);
        dvm_provider_release(reply);
        return NULL;
    }
    files->request = request;
    files->reply = reply;
    files->close = dvm_close_prepare();
    if (!files->close || !(dvm_provider_native_flags(request) & DVM_PROVIDER_CANCEL_PATH) ||
        (!duplex && !(dvm_provider_native_flags(reply) & DVM_PROVIDER_CANCEL_PATH)))
    {
        dvm_close_discard(files->close);
        destroy_files(files);
        return NULL;
    }
    files->duplex = duplex;
    files->references = 1;
    return files;
}


/* EN/RU: Capture both grants before returning a published route.
 * Захватить оба файла до публикации маршрута. */
DVM_ScalarFiles *dvm_scalar_files(DVM_Provider *request, DVM_Provider *reply)
{
    if (!request || !reply || request == reply)
    {
        return NULL;
    }
    DVM_Provider *writer = dvm_provider_capture(request, ".");
    DVM_Provider *reader = dvm_provider_capture(reply, ".");
    return new_files(writer, reader, 0);
}


/* EN/RU: Retain without allowing overflow.
 * Удержать ссылку без переполнения счётчика. */
int dvm_scalar_files_retain(DVM_ScalarFiles *files)
{
    if (!files)
    {
        return 1;
    }
    unsigned current = __atomic_load_n(&files->references, __ATOMIC_RELAXED);
    while (current && current < UINT_MAX)
    {
        if (__atomic_compare_exchange_n(&files->references, &current, current + 1, 1,
                __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: Queue final provider release on the native closer.
 * Поставить окончательное освобождение провайдеров в очередь нативного closer. */
void dvm_scalar_files_release(DVM_ScalarFiles *files)
{
    if (files && __atomic_sub_fetch(&files->references, 1, __ATOMIC_ACQ_REL) == 0)
    {
        DVM_VfsHandle handle = {.ops = &cleanup, .state = files};
        dvm_close_submit_resource(files->close, &handle);
    }
}


/* EN/RU: Detach the native route before retiring the module descriptor.
 * Отсоединить нативный маршрут перед освобождением дескриптора модуля. */
void dvm_scalar_module_retire(Module *module)
{
    DVM_ScalarFiles *files = module->scalar_files;
    module->scalar_files = NULL;
    dvm_scalar_files_release(files);
}


/* EN/RU: Open-only authority lives in the captured file; require advertised rwx first.
 * Полномочия открытия держит захваченный файл. До захвата проверить rwx. */
DVM_ScalarFiles *dvm_scalar_function_files(DVM_Provider *directory, const char *name)
{
    DVM_Provider *file = dvm_provider_capture(directory, name);
    FVM_FileInfo info;
    if (!file || dvm_provider_info(file, ".", &info) != FVM_OK || info.directory ||
        !(info.mode & 0444) || !(info.mode & 0222) || !(info.mode & 0111))
    {
        dvm_provider_release(file);
        return NULL;
    }
    return new_files(file, NULL, 1);
}


/* EN/RU: Encode, write once, rewind, and read bounded chunks from the same opened fid.
 * Закодировать, записать один раз, перемотать и прочитать части с того же fid. */
int dvm_scalar_function_call(DVM_ScalarFiles *files, int64_t argument, int64_t *value)
{
    unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY + 1];
    size_t size;
    DVM_VfsHandle handle = {0};
    if (!styxscalarwrite(STYX_FUNCTION_REQUEST_TAG, STYX_SCALAR_CALL, argument, bytes,
            sizeof(bytes), &size) ||
        dvm_provider_open(files->request, ".", "rw", &handle) != FVM_OK)
    {
        return -1;
    }
    int result = -1;
    if ((handle.native_flags & (DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_SEEK)) ==
        (DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_SEEK))
    {
        DVM_IoResult sent = dvm_vfs_write_chunk(&handle, bytes, size);
        result = 0;
        long position;
        if (sent.status == FVM_OK && (sent.flags & DVM_IO_PREFIX) &&
            !(sent.flags & DVM_IO_UNCERTAIN) && sent.count == size &&
            dvm_vfs_seek(&handle, 0, FVM_FILE_SEEK_START, &position) == FVM_OK)
        {
            size_t used = 0;
            while (used < sizeof(bytes))
            {
                DVM_IoResult got = dvm_vfs_read_chunk(&handle, bytes + used, sizeof(bytes) - used);
                if (got.status != FVM_OK || !(got.flags & DVM_IO_PREFIX) ||
                    (got.flags & DVM_IO_UNCERTAIN) || got.count > sizeof(bytes) - used)
                {
                    break;
                }
                used += got.count;
                if (!got.count || got.stop == DVM_IO_EOF)
                {
                    StyxScalarKind kind;
                    if (styxscalarread(STYX_FUNCTION_REPLY_TAG, bytes, used, &kind, value))
                    {
                        result = kind == STYX_SCALAR_VALUE ? 1 : kind == STYX_SCALAR_ERROR ? 2 : 0;
                    }
                    break;
                }
            }
        }
    }
    dvm_vfs_close(&handle);
    return result;
}
