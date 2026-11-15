/* Copyright (c) 2026 bhgv
 * EN: Native provider dispatch and linear resource ownership; no managed pointers.
 * RU: Native provider dispatch и линейное владение; без managed pointers.
 */
#include "handle.h"
#include <string.h>


/* EN/RU: Borrow serialized file state/arguments; return native info status.
 * Заимствовать serialized file state/arguments; вернуть native info status. */
static FVM_Result file_info(void *state, FVM_FileInfo *output)
{
    return fvm_port_file_info((FVM_PortFile *)state, output);
}


/* EN/RU: Borrow serialized file state/arguments; return native read status.
 * Заимствовать serialized file state/arguments; вернуть native read status. */
static FVM_Result file_read(void *state, void *bytes, size_t size, size_t *received)
{
    return fvm_port_file_read((FVM_PortFile *)state, bytes, size, received);
}


/* EN/RU: Borrow serialized file state/arguments; return native write status.
 * Заимствовать serialized file state/arguments; вернуть native write status. */
static FVM_Result file_write(void *state, const void *bytes, size_t size)
{
    return fvm_port_file_write((FVM_PortFile *)state, bytes, size);
}


/* EN/RU: Borrow serialized file state/arguments; return native seek status.
 * Заимствовать serialized file state/arguments; вернуть native seek status. */
static FVM_Result file_seek(void *state, long offset, int origin, long *position)
{
    return fvm_port_file_seek((FVM_PortFile *)state, offset, origin, position);
}


/* EN/RU: Consume exclusive file after borrowers retire; native I/O only.
 * Поглотить file после ухода заёмщиков; только native I/O. */
static void file_close(void *state)
{
    fvm_port_file_close((FVM_PortFile *)state);
}

static const DVM_VfsOps file_ops = {file_info, file_read, file_write, file_seek, NULL, file_close}; /* Immutable native file provider. */


/* EN/RU: Borrow serialized directory state/arguments; return native info status.
 * Заимствовать serialized directory state/arguments; вернуть native info status. */
static FVM_Result directory_info(void *state, FVM_FileInfo *output)
{
    return fvm_port_directory_info((FVM_PortDirectory *)state, output);
}


/* EN/RU: Borrow serialized directory state/arguments; return native list status.
 * Заимствовать serialized directory state/arguments; вернуть native list status. */
static FVM_Result directory_list(void *state, FVM_FileInfo **entries, size_t *count)
{
    return fvm_port_directory_read((FVM_PortDirectory *)state, entries, count);
}


/* EN/RU: Consume exclusive directory after borrowers retire; native I/O only.
 * Поглотить directory после ухода заёмщиков; только native I/O. */
static void directory_close(void *state)
{
    fvm_port_directory_close((FVM_PortDirectory *)state);
}

static const DVM_VfsOps directory_ops = {directory_info, NULL, NULL, NULL, directory_list, directory_close}; /* Immutable native directory provider. */


/* EN/RU: See handle.h; no callback or allocation during adoption.
 * См. handle.h; без callback/allocation при принятии. */
int dvm_vfs_adopt(DVM_VfsHandle *out, FVM_PortFile *file, FVM_PortDirectory *directory)
{
    if (!out || out->ops || out->state || (file && directory))
    {
        return 0;
    }
    out->ops = file ? &file_ops : directory ? &directory_ops : NULL;
    out->state = file ? (void *)file : (void *)directory;
    return 1;
}


/* EN/RU: See handle.h; linear transfer requires distinct, valid owning values.
 * См. handle.h; перенос требует разные корректные owning values. */
int dvm_vfs_move(DVM_VfsHandle *out, DVM_VfsHandle *source)
{
    if (!out || !source || out == source || out->ops || out->state ||
        (source->ops && (!source->state || !source->ops->close)) || (!source->ops && source->state))
    {
        return 0;
    }
    *out = *source;
    memset(source, 0, sizeof(*source));
    return 1;
}


/* EN/RU: See handle.h; clear before callback prevents repeated consuming close.
 * См. handle.h; очистить до callback для защиты от повторного close. */
void dvm_vfs_close(DVM_VfsHandle *handle)
{
    DVM_VfsHandle owned;
    if (!handle)
    {
        return;
    }
    owned = *handle;
    memset(handle, 0, sizeof(*handle));
    if (owned.ops && owned.ops->close && owned.state)
    {
        owned.ops->close(owned.state);
    }
}


/* EN/RU: See handle.h; invalid/unsupported dispatch never invokes a provider.
 * См. handle.h; invalid/unsupported dispatch не вызывает provider. */
FVM_Result dvm_vfs_info(DVM_VfsHandle *handle, FVM_FileInfo *output)
{
    if (!handle || !handle->ops || !handle->state)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return handle->ops->info ? handle->ops->info(handle->state, output) : FVM_UNSUPPORTED;
}


/* EN/RU: See handle.h; invalid/unsupported dispatch never invokes a provider.
 * См. handle.h; invalid/unsupported dispatch не вызывает provider. */
FVM_Result dvm_vfs_read(DVM_VfsHandle *handle, void *bytes, size_t size, size_t *received)
{
    if (!handle || !handle->ops || !handle->state)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return handle->ops->read ? handle->ops->read(handle->state, bytes, size, received) : FVM_UNSUPPORTED;
}


/* EN/RU: See handle.h; invalid/unsupported dispatch never invokes a provider.
 * См. handle.h; invalid/unsupported dispatch не вызывает provider. */
FVM_Result dvm_vfs_write(DVM_VfsHandle *handle, const void *bytes, size_t size)
{
    if (!handle || !handle->ops || !handle->state)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return handle->ops->write ? handle->ops->write(handle->state, bytes, size) : FVM_UNSUPPORTED;
}


/* EN/RU: See handle.h; invalid/unsupported dispatch never invokes a provider.
 * См. handle.h; invalid/unsupported dispatch не вызывает provider. */
FVM_Result dvm_vfs_seek(DVM_VfsHandle *handle, long offset, int origin, long *position)
{
    if (!handle || !handle->ops || !handle->state)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return handle->ops->seek ? handle->ops->seek(handle->state, offset, origin, position) : FVM_UNSUPPORTED;
}


/* EN/RU: See handle.h; invalid/unsupported dispatch never invokes a provider.
 * См. handle.h; invalid/unsupported dispatch не вызывает provider. */
FVM_Result dvm_vfs_list(DVM_VfsHandle *handle, FVM_FileInfo **entries, size_t *count)
{
    if (!handle || !handle->ops || !handle->state)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return handle->ops->list ? handle->ops->list(handle->state, entries, count) : FVM_UNSUPPORTED;
}


/* EN/RU: See handle.h; only the matching trusted native provider has this layout.
 * См. handle.h; этот layout принадлежит только соответствующему native provider. */
FVM_PortFile *dvm_vfs_file(DVM_VfsHandle *handle)
{
    return handle && handle->ops == &file_ops ? (FVM_PortFile *)handle->state : NULL;
}


/* EN/RU: See handle.h; only the matching trusted native provider has this layout.
 * См. handle.h; этот layout принадлежит только соответствующему native provider. */
FVM_PortDirectory *dvm_vfs_directory(DVM_VfsHandle *handle)
{
    return handle && handle->ops == &directory_ops ? (FVM_PortDirectory *)handle->state : NULL;
}
