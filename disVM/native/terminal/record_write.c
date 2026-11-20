/* Copyright (c) 2026 bhgv
 * EN: Write one complete record through an exact filesystem capability.
 * RU: Записать одну полную запись через точный файловый провайдер.
 */
#include "record_write.h"
#include "cancel.h"

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
