/* Copyright (c) 2026 bhgv
 * EN: Native lifetime for an exact request/reply grant.
 * RU: Нативное владение точными файлами запроса и ответа.
 */
#include "scalar_files.h"
#include "program_view.h"
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "launch_files.h"
#include "../../native/terminal/scalar_record.h"
#include "../../native/terminal/scalar_verify.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Consume route storage and its two file owners.
 * Освободить маршрут и обе его ссылки на файлы. */
static void destroy_files(void *state)
{
    DVM_ScalarFiles *files = state;
    dvm_provider_release(files->request);
    dvm_provider_release(files->reply);
    dvm_scalar_files_release(files->next);
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
    if (!styxscalar_name_length(name))
    {
        return NULL;
    }
    DVM_Provider *file = dvm_provider_capture(directory, name);
    FVM_FileInfo info;
    if (!file || dvm_provider_info(file, ".", &info) != FVM_OK || info.directory ||
        !(info.mode & 0444) || !(info.mode & 0222) || !(info.mode & 0111))
    {
        dvm_provider_release(file);
        return NULL;
    }
    DVM_ScalarFiles *files = new_files(file, NULL, 1);
    if (files)
    {
        strcpy(files->name, name);
    }
    return files;
}


/* EN: Publish the active PID on an explicitly bound reverse session for this call.
 * The native binding owns the lease until the Styx exchange ends.
 * RU: Опубликовать текущий PID во встречной сессии на время вызова.
 * Нативная связь удерживает публикацию до завершения обмена Styx. */
int dvm_scalar_function_invoke(DVM_ScalarFiles *files, DVM_Namespace *group,
    DVM_LaunchFiles *descriptors, uint32_t pid,
    int64_t argument, int64_t *value)
{
    DVM_CallerBinding binding;
    if (!files->caller || !dvm_provider_caller_binding(files->request, &binding))
    {
        return dvm_scalar_function_call(files, pid, argument, value);
    }
    DVM_Provider *tree = dvm_program_current(group, descriptors, pid);
    void *lease = tree ? binding.begin(binding.state, files->request, tree, pid) : NULL;
    if (!lease)
    {
        dvm_provider_release(tree);
        return -1;
    }
    int result = dvm_scalar_function_call(files, pid, argument, value);
    binding.end(binding.state, lease);
    dvm_provider_release(tree);
    return result;
}


/* EN: Exchange one native function frame and return the exact reply.
 * RU: Обменять кадр функции и вернуть точный ответ. */
static int exchange_frame(DVM_ScalarFiles *files, unsigned char *bytes, size_t size, size_t capacity,
    size_t *received)
{
    DVM_VfsHandle handle = {0};
    if (dvm_provider_open(files->request, ".", "rw", &handle) != FVM_OK)
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
            while (used < capacity)
            {
                DVM_IoResult got = dvm_vfs_read_chunk(&handle, bytes + used, capacity - used);
                if (got.status != FVM_OK || !(got.flags & DVM_IO_PREFIX) ||
                    (got.flags & DVM_IO_UNCERTAIN) || got.count > capacity - used)
                {
                    break;
                }
                used += got.count;
                if (!got.count || got.stop == DVM_IO_EOF)
                {
                    *received = used;
                    result = 1;
                    break;
                }
            }
        }
    }
    dvm_vfs_close(&handle);
    return result;
}


/* EN/RU: Encode, write once, rewind, and read bounded chunks from the same opened fid.
 * Закодировать, записать один раз, перемотать и прочитать части с того же fid. */
int dvm_scalar_function_call(DVM_ScalarFiles *files, uint32_t pid, int64_t argument, int64_t *value)
{
    unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY + 1];
    size_t size;
    int wide = files->signature == DVM_SCALAR_BIG_SIGNATURE;
    if (!wide && (argument < INT32_MIN || argument > INT32_MAX))
    {
        return -1;
    }
    int encoded;
    if (files->caller)
    {
        encoded = wide ? styxscalarbigcallerwrite(pid, argument, bytes, sizeof(bytes), &size) :
            styxscalarcallerwrite(pid, (int32_t)argument, bytes, sizeof(bytes), &size);
    }
    else
    {
        encoded = styxscalarwrite(wide ? STYX_FUNCTION_BIG_TAG : STYX_FUNCTION_REQUEST_TAG,
            STYX_SCALAR_CALL, argument, bytes, sizeof(bytes), &size);
    }
    size_t received = 0;
    int exchanged = encoded ? exchange_frame(files, bytes, size, sizeof(bytes), &received) : -1;
    if (exchanged != 1)
    {
        return exchanged;
    }
    StyxScalarKind kind;
    if (!styxscalarread(STYX_FUNCTION_REPLY_TAG, bytes, received, &kind, value))
    {
        return 0;
    }
    return kind == STYX_SCALAR_ERROR ? 2 : kind == STYX_SCALAR_VALUE &&
        (wide || (*value >= INT32_MIN && *value <= INT32_MAX));
}


/* EN: Exchange a composed frame while holding its optional caller lease.
 * RU: Обменять составную запись, удерживая контекст клиента. */
static int composed(DVM_ScalarFiles *files, DVM_Namespace *group, DVM_LaunchFiles *descriptors,
    uint32_t pid, unsigned long tag, const StyxTuple *argument, unsigned char *bytes, size_t capacity,
    size_t *received)
{
    size_t size;
    if (!styxtuplewrite(tag, files->caller ? pid : 0, argument, bytes, capacity, &size))
    {
        return -1;
    }
    DVM_CallerBinding binding;
    DVM_Provider *tree = NULL;
    void *lease = NULL;
    if (files->caller && dvm_provider_caller_binding(files->request, &binding))
    {
        tree = dvm_program_current(group, descriptors, pid);
        lease = tree ? binding.begin(binding.state, files->request, tree, pid) : NULL;
        if (!lease)
        {
            dvm_provider_release(tree);
            return -1;
        }
    }
    int exchanged = exchange_frame(files, bytes, size, capacity, received);
    if (lease)
    {
        binding.end(binding.state, lease);
    }
    dvm_provider_release(tree);
    return exchanged;
}


/* EN: Decode a single tuple result or a verified owner error.
 * RU: Прочитать один кортеж результата или проверенную ошибку владельца. */
int dvm_tuple_function_invoke(DVM_ScalarFiles *files, DVM_Namespace *group,
    DVM_LaunchFiles *descriptors, uint32_t pid, const StyxTuple *argument, StyxTuple *result)
{
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY + 1];
    size_t received = 0;
    int exchanged = composed(files, group, descriptors, pid,
        files->caller ? STYX_TUPLE_CALLER_TAG : STYX_TUPLE_REQUEST_TAG, argument, bytes, sizeof(bytes), &received);
    if (exchanged != 1)
    {
        return exchanged;
    }
    uint32_t response_pid;
    if (styxtupleread(STYX_TUPLE_REPLY_TAG, bytes, received, &response_pid, result))
    {
        return 1;
    }
    StyxScalarKind kind;
    int64_t code;
    return styxscalarread(STYX_FUNCTION_REPLY_TAG, bytes, received, &kind, &code) &&
        kind == STYX_SCALAR_ERROR ? 2 : 0;
}


/* EN: Decode the ordinary result and copy-back as one verified response.
 * RU: Прочитать результат и обновление как единый проверенный ответ. */
int dvm_ref_function_invoke(DVM_ScalarFiles *files, DVM_Namespace *group,
    DVM_LaunchFiles *descriptors, uint32_t pid, const StyxTuple *argument, StyxRefReply *result)
{
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY + 1];
    size_t received = 0;
    int exchanged = composed(files, group, descriptors, pid,
        files->caller ? STYX_REF_CALLER_TAG : STYX_REF_REQUEST_TAG, argument, bytes, sizeof(bytes), &received);
    if (exchanged != 1)
    {
        return exchanged;
    }
    if (styxrefreplyread(bytes, received, result))
    {
        return 1;
    }
    StyxScalarKind kind;
    int64_t code;
    return styxscalarread(STYX_FUNCTION_REPLY_TAG, bytes, received, &kind, &code) &&
        kind == STYX_SCALAR_ERROR ? 2 : 0;
}



/* EN/RU: See scalar_files.h. / См. scalar_files.h. */
int dvm_call_list_function_invoke(DVM_ScalarFiles *files, const StyxCallValues *arguments, StyxCallValues *result)
{
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY + 1];
    size_t size, received = 0;
    if (files->caller || !styxcallrecordwrite(STYX_CALL_LIST_REQUEST_TAG, styxcall_list_args,
            arguments, bytes, sizeof(bytes), &size))
    {
        return -1;
    }
    int exchanged = exchange_frame(files, bytes, size, sizeof(bytes), &received);
    if (exchanged != 1)
    {
        return exchanged;
    }
    if (styxcallrecordread(STYX_CALL_LIST_REPLY_TAG, styxcall_list_result, bytes, received, result))
    {
        return 1;
    }
    StyxScalarKind kind;
    int64_t code;
    return styxscalarread(STYX_FUNCTION_REPLY_TAG, bytes, received, &kind, &code) &&
        kind == STYX_SCALAR_ERROR ? 2 : 0;
}



/* EN/RU: See scalar_files.h. / См. scalar_files.h. */
int dvm_ref_pair_invoke(DVM_ScalarFiles *files, StyxRefPair *value)
{
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY + 1];
    size_t size, received = 0;
    if (files->caller || !styxrefpairwrite(STYX_REF_PAIR_REQUEST_TAG, value, bytes, sizeof(bytes), &size))
    {
        return -1;
    }
    int exchanged = exchange_frame(files, bytes, size, sizeof(bytes), &received);
    if (exchanged != 1)
    {
        return exchanged;
    }
    StyxRefPair result;
    if (styxrefpairread(STYX_REF_PAIR_REPLY_TAG, bytes, received, &result) && result.count == value->count)
    {
        *value = result;
        return 1;
    }
    StyxScalarKind kind;
    int64_t code;
    return styxscalarread(STYX_FUNCTION_REPLY_TAG, bytes, received, &kind, &code) &&
        kind == STYX_SCALAR_ERROR ? 2 : 0;
}
