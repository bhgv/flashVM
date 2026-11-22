/* Copyright (c) 2026 bhgv
 * EN: Encode and deliver one launch record over an explicit file capability.
 * RU: Кодирование и доставка одной записи запуска по выданному файловому полномочию.
 */
#include "record_write.h"
#include "../launch/program_record.h"
#include "flashvm/srecord.h"
#include "cancel.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct LaunchFile
{
    DVM_Provider *provider; /* Owned exact request file, independent of VFS aliases. */
    DVM_LaunchProgram program; /* Optional copied client context. */
    int wait; /* Read completion from the same open request. */
    unsigned long tag;      /* Configured application launch route. */
} LaunchFile;


/* EN/RU: Drop the retained request node without an I/O wait.
 * Освободить ссылку на узел запроса без ожидания I/O. */
static void release(void *opaque)
{
    LaunchFile *file = opaque;
    dvm_provider_release(file->provider);
    free(file);
}


/* EN: Submit once on a read/write fid and keep it through completion.
 * RU: Подать запрос один раз и удерживать тот же fid до завершения. */
static DVM_LaunchDelivery exchange(LaunchFile *file, const void *bytes, size_t size)
{
    DVM_VfsHandle handle = {0};
    DVM_LaunchDelivery delivery = DVM_LAUNCH_NOT_SENT;
    if (dvm_cancel_requested(dvm_cancel_current()) ||
        dvm_provider_open(file->provider, ".", "rw", &handle) != FVM_OK)
    {
        return delivery;
    }
    if (handle.native_flags & DVM_VFS_CANCEL_DATA)
    {
        DVM_IoResult sent = dvm_vfs_write_chunk(&handle, bytes, size);
        if (sent.status == FVM_OK && (sent.flags & DVM_IO_PREFIX) && sent.count == size &&
            !(sent.flags & DVM_IO_UNCERTAIN))
        {
            delivery = DVM_LAUNCH_UNKNOWN;
            long offset;
            if (dvm_vfs_seek(&handle, 0, FVM_FILE_SEEK_START, &offset) == FVM_OK && !offset)
            {
                const struct timespec pause = {0, 10000000};
                while (!dvm_cancel_requested(dvm_cancel_current()))
                {
                    char status[8];
                    DVM_IoResult got = dvm_vfs_read_chunk(&handle, status, sizeof(status));
                    if (got.status != FVM_OK || !(got.flags & DVM_IO_PREFIX) || (got.flags & DVM_IO_UNCERTAIN))
                    {
                        break;
                    }
                    if (got.count)
                    {
                        delivery = got.count == 3 && !memcmp(status, "ok\n", 3) ? DVM_LAUNCH_DELIVERED :
                            got.count == 6 && !memcmp(status, "error\n", 6) ? DVM_LAUNCH_FAILED :
                            DVM_LAUNCH_UNKNOWN;
                        break;
                    }
                    nanosleep(&pause, NULL);
                }
            }
        }
        else if ((sent.flags & DVM_IO_UNCERTAIN) || sent.count || sent.stop == DVM_IO_INVALID)
        {
            delivery = DVM_LAUNCH_UNKNOWN;
        }
    }
    dvm_vfs_close(&handle);
    return delivery;
}


/* EN/RU: Encode bytes and attempt one complete write, preserving uncertain results.
 * Закодировать аргументы и выполнить одну запись, сохранив неопределённый исход. */
static DVM_LaunchDelivery send_frame(LaunchFile *file, const DVM_LaunchProgram *program, const void *arguments, size_t size)
{
    unsigned char header[STYX_LAUNCH_PROGRAM_HEADER];
    size_t prefix = program && program->pid ? sizeof(header) : 0;
    if (prefix && (size > DVM_LAUNCH_ARGUMENT_LIMIT - prefix ||
        !styxlaunchprogramencode(header, program)))
    {
        return DVM_LAUNCH_NOT_SENT;
    }
    size_t capacity = size + prefix + 32;
    unsigned char *bytes = malloc(capacity);
    size_t used;
    DVM_LaunchDelivery delivery = DVM_LAUNCH_NOT_SENT;
    if (bytes && (prefix ? fvm_srecord_write_pair(file->tag, header, prefix, arguments, size, bytes, capacity, &used) :
        fvm_srecord_write(file->tag, arguments, size, bytes, capacity, &used)) == FVM_S_OK)
    {
        delivery = file->wait ? exchange(file, bytes, used) : styxterminalrecordwrite(file->provider, bytes, used);
    }
    free(bytes);
    return delivery;
}


/* EN/RU: Send the existing unscoped or explicitly selected frame.
 * Отправить прежнюю запись или запись с явно выбранным процессом. */
static DVM_LaunchDelivery send(void *opaque, const void *arguments, size_t size)
{
    LaunchFile *file = opaque;
    return send_frame(file, &file->program, arguments, size);
}


/* EN/RU: Copy the process selector on each send with no shared mutable state.
 * Скопировать выбор процесса при отправке, без общего изменяемого состояния. */
static DVM_LaunchDelivery send_process(void *opaque, uint32_t pid, const void *arguments, size_t size)
{
    LaunchFile *file = opaque;
    DVM_LaunchProgram program = {pid, 0, 0};
    return send_frame(file, &program, arguments, size);
}


/* EN/RU: See launch_send.h; do not open the provider until actual submission.
 * См. launch_send.h; до подачи запроса не открывать provider. */
static DVM_LaunchAddress *address_for(DVM_Provider *request_file, unsigned long tag,
    const DVM_LaunchProgram *program, int automatic, int wait)
{
    if (!request_file || !(dvm_provider_native_flags(request_file) & DVM_PROVIDER_CANCEL_PATH) || !tag || tag > FVM_SRECORD_U32)
    {
        return NULL;
    }
    LaunchFile *file = calloc(1, sizeof(*file));
    if (!file || !dvm_provider_retain(request_file))
    {
        free(file);
        return NULL;
    }
    file->provider = request_file;
    file->tag = tag;
    file->wait = wait;
    if (program)
    {
        file->program = *program;
    }
    DVM_LaunchAddress *address = automatic ? dvm_launch_address_process_transport(send_process, file, release) :
        dvm_launch_address_transport(send, file, release);
    if (!address)
    {
        release(file);
    }
    return address;
}


/* EN/RU: Create the original launch address with raw startup payloads.
 * Создать прежний адрес запуска с исходными данными запуска. */
DVM_LaunchAddress *styxterminallaunchaddress(DVM_Provider *request_file, unsigned long tag)
{
    return address_for(request_file, tag, NULL, 0, 0);
}


/* EN/RU: Retain the route and copy the process selector for subsequent sends.
 * Удержать маршрут и скопировать выбор процесса для последующих отправок. */
DVM_LaunchAddress *styxterminallaunchprogramaddress(DVM_Provider *request_file, unsigned long tag,
    const DVM_LaunchProgram *program)
{
    return program && program->pid && program->pid != program->parent_pid ?
        address_for(request_file, tag, program, 0, 0) : NULL;
}


/* EN/RU: Create a Styx address that takes the active caller PID on each send.
 * Создать адрес Styx, получающий PID активного вызывающего при каждой отправке. */
DVM_LaunchAddress *styxterminallaunchcalleraddress(DVM_Provider *request_file, unsigned long tag)
{
    return address_for(request_file, tag, NULL, 1, 0);
}

/* EN/RU: See launch_send.h. / См. launch_send.h. */
DVM_LaunchAddress *styxterminallaunchwaitaddress(DVM_Provider *request_file, unsigned long tag)
{
    return address_for(request_file, tag, NULL, 1, 1);
}
