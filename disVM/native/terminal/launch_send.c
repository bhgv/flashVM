/* Copyright (c) 2026 bhgv
 * EN: Encode and deliver one launch record over an explicit file capability.
 * RU: Кодирование и доставка одной записи запуска по выданному файловому полномочию.
 */
#include "record_write.h"
#include "flashvm/srecord.h"
#include "cancel.h"
#include <stdlib.h>

typedef struct LaunchFile
{
    DVM_Provider *provider; /* Owned exact request file, independent of VFS aliases. */
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


/* EN/RU: See record_write.h; a split or uncertain write is never replayed.
 * См. record_write.h; частичная запись или неизвестный исход не повторяются. */
DVM_LaunchDelivery styxterminalrecordwrite(DVM_Provider *provider, const void *bytes, size_t size)
{
    DVM_VfsHandle handle = {0};
    DVM_LaunchDelivery delivery = DVM_LAUNCH_NOT_SENT;
    if (provider && bytes && size && !dvm_cancel_requested(dvm_cancel_current()) &&
        (!dvm_cancel_current() || (dvm_provider_native_flags(provider) & DVM_PROVIDER_CANCEL_PATH)) &&
        dvm_provider_open(provider, ".", "wo", &handle) == FVM_OK &&
        (!dvm_cancel_current() || (handle.native_flags & DVM_VFS_CANCEL_DATA)))
    {
        DVM_IoResult result = dvm_vfs_write_chunk(&handle, bytes, size);
        if (result.status == FVM_OK && (result.flags & DVM_IO_PREFIX) && result.count == size &&
            !(result.flags & DVM_IO_UNCERTAIN))
        {
            delivery = DVM_LAUNCH_DELIVERED;
        }
        else if ((result.flags & DVM_IO_UNCERTAIN) || result.count || result.stop == DVM_IO_INVALID)
        {
            delivery = DVM_LAUNCH_UNKNOWN;
        }
    }
    dvm_vfs_close(&handle);
    return delivery;
}


/* EN/RU: Encode bytes and attempt one complete write, preserving uncertain results.
 * Закодировать аргументы и выполнить одну запись, сохранив неопределённый исход. */
static DVM_LaunchDelivery send(void *opaque, const void *arguments, size_t size)
{
    LaunchFile *file = opaque;
    size_t capacity = size + 32;
    unsigned char *bytes = malloc(capacity);
    size_t used;
    DVM_LaunchDelivery delivery = DVM_LAUNCH_NOT_SENT;
    if (bytes && fvm_srecord_write(file->tag, arguments, size, bytes, capacity, &used) == FVM_S_OK)
    {
        delivery = styxterminalrecordwrite(file->provider, bytes, used);
    }
    free(bytes);
    return delivery;
}


/* EN/RU: See launch_send.h; do not open the provider until actual submission.
 * См. launch_send.h; до подачи запроса не открывать provider. */
DVM_LaunchAddress *styxterminallaunchaddress(DVM_Provider *request_file, unsigned long tag)
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
    DVM_LaunchAddress *address = dvm_launch_address_transport(send, file, release);
    if (!address)
    {
        release(file);
    }
    return address;
}
