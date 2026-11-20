/* Copyright (c) 2026 bhgv
 * EN: Expose imported numbered descriptors using their existing file slots and FIFO.
 * RU: Публикация импортированных дескрипторов через их слоты и очередь операций.
 */
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "disvm/native_context.h"
#include "interp.h"
#include "runt.h"
#include "launch_files.h"
#include "vfs/export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct FdGrant
{
    int slot; /* Shared launch slot, retained by the owning provider. */
    unsigned access; /* Original read/write ceiling. */
} FdGrant;


/* EN: Borrow the slot only after entering its shared FIFO. The provider pins it.
 * RU: Заимствовать слот после входа в общую очередь. Провайдер удерживает слот. */
static DVM_VfsHandle *enter(FdGrant *grant, DVM_FileGate **gate, DVM_FileTicket *ticket)
{
    *gate = dvm_runtime_file_gate(grant->slot);
    DVM_VfsHandle *handle = dvm_runtime_resource(grant->slot);
    if (!*gate || !handle)
    {
        return NULL;
    }
    dvm_file_gate_reserve(*gate, ticket);
    return dvm_file_gate_enter_now(*gate, ticket) ? handle : NULL;
}


/* EN: Give a standalone Styx worker an owner for one gated operation.
 * RU: Создать владельца операции на отдельном потоке Styx. */
static int ensure_owner(DVM_NativeContext **temporary)
{
    *temporary = NULL;
    if (dvm_native_context_current())
    {
        return 1;
    }
    DVM_NativeContext *owner = dvm_native_context_create(sizeof(REG), NULL);
    if (!owner || !dvm_native_context_bind(owner))
    {
        if (owner)
        {
            dvm_native_context_destroy(owner);
        }
        return 0;
    }
    *temporary = owner;
    return 1;
}


/* EN: Retire a temporary owner after leaving the gate.
 * RU: Освободить временного владельца после выхода из очереди. */
static void release_owner(DVM_NativeContext *temporary)
{
    if (temporary)
    {
        dvm_native_context_unbind(temporary);
        dvm_native_context_destroy(temporary);
    }
}


/* EN: Read from the existing file position, preserving the inner outcome.
 * RU: Читать с общей позиции, сохраняя исходный результат. */
static void read_file(void *state, void *bytes, size_t count, DVM_IoResult *result)
{
    FdGrant *grant = state;
    DVM_NativeContext *temporary;
    DVM_FileGate *gate;
    DVM_FileTicket ticket = {0};
    *result = (DVM_IoResult){.status = FVM_ERROR};
    if (!(grant->access & FVM_ACCESS_READ) || !ensure_owner(&temporary))
    {
        return;
    }
    DVM_VfsHandle *file = enter(grant, &gate, &ticket);
    if (file)
    {
        *result = dvm_vfs_read_chunk(file, bytes, count);
        dvm_file_gate_leave(gate, &ticket);
    }
    release_owner(temporary);
}


/* EN: Write at the existing file position, preserving short writes.
 * RU: Писать с общей позиции, сохраняя неполные записи. */
static void write_file(void *state, const void *bytes, size_t count, DVM_IoResult *result)
{
    FdGrant *grant = state;
    DVM_NativeContext *temporary;
    DVM_FileGate *gate;
    DVM_FileTicket ticket = {0};
    *result = (DVM_IoResult){.status = FVM_ERROR};
    if (!(grant->access & FVM_ACCESS_WRITE) || !ensure_owner(&temporary))
    {
        return;
    }
    DVM_VfsHandle *file = enter(grant, &gate, &ticket);
    if (file)
    {
        *result = dvm_vfs_write_chunk(file, bytes, count);
        dvm_file_gate_leave(gate, &ticket);
    }
    release_owner(temporary);
}


/* EN: Describe the descriptor endpoint without changing its open position.
 * RU: Описать точку дескриптора без изменения позиции. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    FdGrant *grant = state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    out->mode = ((grant->access & FVM_ACCESS_READ) ? 0444 : 0) |
        ((grant->access & FVM_ACCESS_WRITE) ? 0222 : 0);
    return FVM_OK;
}


/* EN: Describe an open alias using the retained grant.
 * RU: Описать открытый псевдоним по удерживаемому разрешению. */
static FVM_Result handle_info(void *state, FVM_FileInfo *out)
{
    return info(state, ".", out);
}


/* EN: An alias is borrowed through the handle's factory reference.
 * RU: Псевдоним заимствован через ссылку дескриптора на фабрику. */
static void alias_close(void *state)
{
    (void)state;
}

static const DVM_VfsOps read_ops = {.info = handle_info, .read_chunk = read_file, .close = alias_close}; /* Read-only alias. */
static const DVM_VfsOps write_ops = {.info = handle_info, .write_chunk = write_file, .close = alias_close}; /* Write-only alias. */
static const DVM_VfsOps both_ops = {.info = handle_info, .read_chunk = read_file, .write_chunk = write_file,
    .close = alias_close}; /* Read/write alias. */


/* EN: Check the requested access against the imported open mode.
 * RU: Сверить запрошенный доступ с импортированным режимом открытия. */
static FVM_Result access_check(void *state, const char *path, unsigned access)
{
    FdGrant *grant = state;
    return !strcmp(path, ".") && !(access & ~grant->access) ? FVM_OK : FVM_ERROR;
}


/* EN: Create an alias without reopening the original path or truncating.
 * RU: Создать псевдоним без повторного открытия пути или усечения. */
static FVM_Result open_alias(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    unsigned access = !strcmp(mode, "r") ? FVM_ACCESS_READ : !strcmp(mode, "wo") ? FVM_ACCESS_WRITE :
        !strcmp(mode, "rw") ? FVM_ACCESS_READ | FVM_ACCESS_WRITE : 0;
    if (!access || access_check(state, path, access) != FVM_OK)
    {
        return FVM_ERROR;
    }
    out->ops = access == FVM_ACCESS_READ ? &read_ops : access == FVM_ACCESS_WRITE ? &write_ops : &both_ops;
    out->state = state;
    out->native_flags = DVM_VFS_CANCEL_INFO | DVM_VFS_CANCEL_CLOSE;
    return FVM_OK;
}


/* EN: Drop the retained slot after the last alias and provider close.
 * RU: Освободить слот после закрытия последнего псевдонима и провайдера. */
static void destroy_grant(void *state)
{
    FdGrant *grant = state;
    dvm_file_slots_drop_launch(grant->slot);
    free(grant);
}

static const DVM_ProviderOps factory = {.info = info, .open = open_alias, .destroy = destroy_grant,
    .flags = DVM_PROVIDER_STABLE_BIND, .access = access_check}; /* Immutable exact FD grant. */


/* EN: Retain one imported slot and map it to its decimal number.
 * RU: Удержать импортированный слот и отобразить его десятичный номер. */
static int map_fd(DVM_View *view, unsigned number, int slot, unsigned access)
{
    FdGrant *grant = calloc(1, sizeof(*grant));
    if (!grant || !dvm_file_slots_retain_launch(slot))
    {
        free(grant);
        return 0;
    }
    grant->slot = slot;
    grant->access = access;
    DVM_Provider *file = dvm_provider_create_native(&factory, grant, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    if (!file)
    {
        destroy_grant(grant);
        return 0;
    }
    char path[sizeof("/31")];
    snprintf(path, sizeof(path), "/%u", number);
    int mapped = dvm_view_map(view, path, file, ".", access, 0);
    dvm_provider_release(file);
    return mapped;
}


/* EN/RU: See launch_files.h. Publish the directory only after mapping all slots.
 * См. launch_files.h. Опубликовать каталог после отображения всех слотов. */
DVM_Provider *dvm_launch_fd_view(DVM_LaunchFiles *files, int *success)
{
    DVM_View *view = NULL;
    *success = 0;
    for (unsigned number = 0; number < DVM_LAUNCH_FD_COUNT; number++)
    {
        unsigned access;
        int slot = dvm_launch_file_lookup(files, number, &access);
        if (slot < 0)
        {
            continue;
        }
        if (!view)
        {
            view = dvm_view_create(NULL, NULL, NULL);
        }
        if (!view || !map_fd(view, number, slot, access))
        {
            dvm_view_discard(view);
            return NULL;
        }
    }
    DVM_Provider *directory = view ? dvm_view_freeze(view) : NULL;
    *success = !view || directory != NULL;
    return directory;
}
