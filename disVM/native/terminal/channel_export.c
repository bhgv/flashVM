/* Copyright (c) 2026 bhgv
 * EN: Route provider-owned channel operations under an exact, leased function fid.
 * RU: Маршрутизация операций каналов провайдера в пределах аренды точного fid вызова.
 */
#include "channel_export.h"
#include "provider_internal.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHANNEL_EXPORT_LIMIT 128U /* Maximum retained function fids per module. */

typedef struct ChannelExport ChannelExport; /* Native control attached to an active function fid. */


/* EN: Scope random capability lookup to one mounted module on one session.
 * RU: Ограничить поиск случайного ключа одним каталогом модуля на одной сессии. */
struct StyxChannelExports
{
    FVM_PortSync *lock; /* Serializes publication, lookup, and clunk. */
    ChannelExport *head; /* Borrowed controls pinned by their function fids. */
    size_t refs; /* Server reference and controls, protected by lock. */
    int closed; /* Server no longer admits controls. */
    size_t count; /* Includes unpublished reserved leases. */
};


/* EN: Separate the long-lived forward lease from the call's reverse authority.
 * RU: Отделить долгоживущую прямую аренду от встречного доступа вызова. */
struct ChannelExport
{
    StyxChannelExports *registry; /* Owned registry reference until final destruction. */
    int linked; /* Registry lock protects membership. */
    DVM_CancelTap *parent; /* Owned propagation from the parent lease. */
    ChannelExport *next; /* Borrowed next entry, under the registry lock. */
    FVM_PortSync *lock; /* Protects the published provider and revocation. */
    DVM_Provider *bus; /* Owned native operation provider while published. */
    DVM_Cancel *cancel; /* Sticky forward-fid cancellation, retained until last user. */
    unsigned char token[STYX_CHANNEL_EXPORT_TOKEN_SIZE]; /* Immutable random lookup secret. */
};


/* EN: Route one independent send/recv/alt fid only after reading its token.
 * RU: Связать отдельный fid send/recv/alt только после чтения ключа. */
typedef struct ExportOperation
{
    StyxChannelExports *registry; /* Borrowed while the server's operation fid lives. */
    DVM_Cancel *cancel; /* Owned revocation source copied from the selected control. */
    DVM_VfsHandle operation; /* Owned opened bus fid, independent of the lease fid. */
    unsigned char token[STYX_CHANNEL_EXPORT_TOKEN_SIZE]; /* Staging until the prefix is complete. */
    size_t staged; /* Bytes admitted into token. */
    int rejected; /* An invalid authorization permanently retires this operation. */
} ExportOperation;


/* EN: Read one complete unpredictable lease token before publishing the control.
 * RU: Прочитать полный непредсказуемый ключ до публикации управления. */
static int random_token(unsigned char token[STYX_CHANNEL_EXPORT_TOKEN_SIZE])
{
    int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd < 0)
    {
        return 0;
    }
    size_t used = 0;
    while (used < STYX_CHANNEL_EXPORT_TOKEN_SIZE)
    {
        ssize_t got = read(fd, token + used, STYX_CHANNEL_EXPORT_TOKEN_SIZE - used);
        if (got < 0 && errno == EINTR)
        {
            continue;
        }
        if (got <= 0)
        {
            break;
        }
        used += (size_t)got;
    }
    close(fd);
    return used == STYX_CHANNEL_EXPORT_TOKEN_SIZE;
}


/* EN/RU: See channel_export.h. / См. channel_export.h. */
StyxChannelExports *styxchannel_exports_create(void)
{
    StyxChannelExports *exports = calloc(1, sizeof(*exports));
    if (exports && !(exports->lock = fvm_port_sync_create()))
    {
        free(exports);
        exports = NULL;
    }
    if (exports)
    {
        exports->refs = 1;
    }
    return exports;
}


/* EN: Release an owned registry reference after all its controls are unlinked.
 * RU: Снять свою ссылку на реестр после снятия его записей. */
static void release_registry(StyxChannelExports *exports)
{
    if (exports)
    {
        fvm_port_sync_lock(exports->lock);
        int last = --exports->refs == 0;
        fvm_port_sync_unlock(exports->lock);
        if (last)
        {
            fvm_port_sync_destroy(exports->lock);
            free(exports);
        }
    }
}


/* EN/RU: See channel_export.h. / См. channel_export.h. */
void styxchannel_exports_destroy(StyxChannelExports *exports)
{
    if (exports)
    {
        fvm_port_sync_lock(exports->lock);
        exports->closed = 1;
        fvm_port_sync_unlock(exports->lock);
        release_registry(exports);
    }
}


/* EN: Refuse guest traversal of this private control capability.
 * RU: Запретить гостевой обход служебного объекта управления. */
static FVM_Result private_info(void *state, const char *path, FVM_FileInfo *info)
{
    (void)state;
    (void)path;
    (void)info;
    return FVM_UNSUPPORTED;
}


/* EN: Refuse guest opens; the control is used through native metadata only.
 * RU: Отклонить открытие гостем. Управление доступно по нативным метаданным. */
static FVM_Result private_open(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)state;
    (void)path;
    (void)mode;
    (void)out;
    return FVM_UNSUPPORTED;
}


/* EN: Unlink an owned control exactly once. Pins keep the registry and bus alive.
 * RU: Снять свою запись ровно один раз. Ссылки удерживают реестр и UniBus. */
static void revoke_control(ChannelExport *control)
{
    StyxChannelExports *exports = control->registry;
    if (!exports)
    {
        return;
    }
    fvm_port_sync_lock(exports->lock);
    int linked = control->linked;
    if (linked)
    {
        ChannelExport **entry = &exports->head;
        while (*entry != control)
        {
            entry = &(*entry)->next;
        }
        *entry = control->next;
        control->linked = 0;
        exports->count--;
    }
    fvm_port_sync_unlock(exports->lock);
    if (linked)
    {
        dvm_cancel_request(control->cancel);
        fvm_port_sync_lock(control->lock);
        DVM_Provider *bus = control->bus;
        control->bus = NULL;
        fvm_port_sync_unlock(control->lock);
        dvm_provider_release(bus);
    }
}


/* EN: Consume native state after its registered function fid has retired.
 * RU: Освободить нативное состояние после закрытия зарегистрированного fid. */
static void destroy_control(void *state)
{
    ChannelExport *control = state;
    revoke_control(control);
    dvm_cancel_unsubscribe(control->parent);
    release_registry(control->registry);
    dvm_provider_release(control->bus);
    dvm_cancel_release(control->cancel);
    if (control->lock)
    {
        fvm_port_sync_destroy(control->lock);
    }
    free(control);
}


static const DVM_ProviderOps control_ops = {.info = private_info, .open = private_open,
    .destroy = destroy_control}; /* Immutable type identity for private controls. */


/* EN: Return borrowed control storage only for the exact native provider type.
 * RU: Вернуть заимствованное состояние только провайдера точного нативного типа. */
static ChannelExport *state_of(DVM_Provider *provider)
{
    return provider && provider->ops == &control_ops ? provider->state : NULL;
}


/* EN: Create a control, checking optional parent membership under the registry lock.
 * RU: Создать управление, проверив наличие родителя под замком реестра. */
static DVM_Provider *create_control(StyxChannelExports *exports, ChannelExport *parent)
{
    ChannelExport *control = exports ? calloc(1, sizeof(*control)) : NULL;
    if (!control)
    {
        return NULL;
    }
    control->lock = fvm_port_sync_create();
    control->cancel = dvm_cancel_create();
    if (!control->lock || !control->cancel || !random_token(control->token) ||
        (parent && !(control->parent = dvm_cancel_forward(parent->cancel, control->cancel))))
    {
        destroy_control(control);
        return NULL;
    }
    DVM_Provider *provider = dvm_provider_create_native(&control_ops, control, NULL, 1, 0);
    if (!provider)
    {
        destroy_control(control);
        return NULL;
    }
    fvm_port_sync_lock(exports->lock);
    int admitted = !exports->closed && exports->count < CHANNEL_EXPORT_LIMIT &&
        (!parent || (parent->linked && !dvm_cancel_requested(parent->cancel)));
    for (const ChannelExport *entry = exports->head; admitted && entry; entry = entry->next)
    {
        admitted = memcmp(entry->token, control->token, sizeof(control->token)) != 0;
    }
    if (admitted)
    {
        exports->refs++;
        control->registry = exports;
        control->linked = 1;
        control->next = exports->head;
        exports->head = control;
        exports->count++;
    }
    fvm_port_sync_unlock(exports->lock);
    if (!admitted)
    {
        dvm_provider_release(provider);
        return NULL;
    }
    return provider;
}


/* EN/RU: See channel_export.h. / См. channel_export.h. */
DVM_Provider *styxchannel_export_create(StyxChannelExports *exports)
{
    return create_control(exports, NULL);
}


/* EN/RU: See channel_export.h. / См. channel_export.h. */
DVM_Provider *styxchannel_export_child(DVM_Provider *provider)
{
    ChannelExport *parent = state_of(provider);
    return parent ? create_control(parent->registry, parent) : NULL;
}


/* EN/RU: See channel_export.h. / См. channel_export.h. */
void styxchannel_export_revoke(DVM_Provider *provider)
{
    ChannelExport *control = state_of(provider);
    if (control)
    {
        revoke_control(control);
    }
}


/* EN/RU: See channel_export.h. / См. channel_export.h. */
int styxchannel_export_token(DVM_Provider *provider, unsigned char token[STYX_CHANNEL_EXPORT_TOKEN_SIZE])
{
    const ChannelExport *control = state_of(provider);
    if (!control || dvm_cancel_requested(control->cancel))
    {
        return 0;
    }
    memcpy(token, control->token, sizeof(control->token));
    return 1;
}


/* EN/RU: See channel_export.h. / См. channel_export.h. */
DVM_Cancel *styxchannel_export_cancel(DVM_Provider *provider)
{
    const ChannelExport *control = state_of(provider);
    return control && dvm_cancel_retain(control->cancel) ? control->cancel : NULL;
}


/* EN/RU: See channel_export.h. / См. channel_export.h. */
int styxchannel_export_publish(DVM_Provider *provider, DVM_Provider *bus)
{
    ChannelExport *control = state_of(provider);
    if (!control || !bus)
    {
        return 0;
    }
    fvm_port_sync_lock(control->lock);
    int published = !control->bus && !dvm_cancel_requested(control->cancel) && dvm_provider_retain(bus);
    if (published)
    {
        control->bus = bus;
    }
    fvm_port_sync_unlock(control->lock);
    return published;
}


/* EN: Check the supplied opaque token without an early exit on matching prefixes.
 * RU: Сверить непрозрачный ключ без раннего выхода по совпавшей части. */
static int token_matches(const unsigned char *left, const unsigned char *right)
{
    unsigned diff = 0;
    for (size_t i = 0; i < STYX_CHANNEL_EXPORT_TOKEN_SIZE; i++)
    {
        diff |= left[i] ^ right[i];
    }
    return diff == 0;
}


/* EN: Acquire one open bus handle and a retained revocation token.
 * RU: Получить один открытый handle шины и удерживаемый токен отзыва. */
static int bind_operation(ExportOperation *operation)
{
    StyxChannelExports *exports = operation->registry;
    DVM_Provider *bus = NULL;
    fvm_port_sync_lock(exports->lock);
    for (ChannelExport *entry = exports->head; entry; entry = entry->next)
    {
        if (token_matches(entry->token, operation->token))
        {
            fvm_port_sync_lock(entry->lock);
            if (entry->bus && !dvm_cancel_requested(entry->cancel) && dvm_provider_retain(entry->bus))
            {
                bus = entry->bus;
                if (dvm_cancel_retain(entry->cancel))
                {
                    operation->cancel = entry->cancel;
                }
            }
            fvm_port_sync_unlock(entry->lock);
            break;
        }
    }
    fvm_port_sync_unlock(exports->lock);
    int opened = operation->cancel && dvm_provider_open(bus, ".", "rw", &operation->operation) == FVM_OK;
    dvm_provider_release(bus);
    return opened;
}


/* EN: Stage exactly one token, then forward admitted bytes to the selected bus.
 * RU: Принять один ключ и направить оставшиеся байты в выбранную шину. */
static void operation_write(void *state, const void *data, size_t size, DVM_IoResult *out)
{
    ExportOperation *operation = state;
    const unsigned char *bytes = data;
    *out = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
    if (operation->rejected || dvm_cancel_requested(operation->cancel))
    {
        return;
    }
    size_t prefix = sizeof(operation->token) - operation->staged;
    if (prefix)
    {
        if (prefix > size)
        {
            prefix = size;
        }
        memcpy(operation->token + operation->staged, bytes, prefix);
        operation->staged += prefix;
        if (operation->staged == sizeof(operation->token) && !bind_operation(operation))
        {
            operation->rejected = 1;
            return;
        }
    }
    if (prefix == size)
    {
        *out = (DVM_IoResult){.status = FVM_OK, .count = prefix, .flags = DVM_IO_PREFIX};
    }
    else
    {
        *out = dvm_vfs_write_chunk(&operation->operation, bytes + prefix, size - prefix);
        if (out->status == FVM_OK)
        {
            out->count += prefix;
        }
    }
}


/* EN: Read only from the admitted operation while its lease still lives.
 * RU: Читать только из допущенной операции, пока действует её аренда. */
static void operation_read(void *state, void *bytes, size_t capacity, DVM_IoResult *out)
{
    ExportOperation *operation = state;
    if (!operation->operation.ops || dvm_cancel_requested(operation->cancel))
    {
        *out = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
        return;
    }
    *out = dvm_vfs_read_chunk(&operation->operation, bytes, capacity);
}


/* EN: Close the operation, leaving the module's original lease alone.
 * RU: Закрыть операцию, не затрагивая исходную аренду модуля. */
static void operation_close(void *state)
{
    ExportOperation *operation = state;
    dvm_vfs_close(&operation->operation);
    dvm_cancel_release(operation->cancel);
    free(operation);
}


/* EN/RU: See channel_export.h. / См. channel_export.h. */
int styxchannel_export_open(StyxChannelExports *exports, DVM_VfsHandle *handle)
{
    static const DVM_VfsOps ops = {.read_chunk = operation_read, .write_chunk = operation_write,
        .close = operation_close}; /* Native protocol operations, no managed heap access. */
    ExportOperation *operation = exports ? calloc(1, sizeof(*operation)) : NULL;
    if (!operation)
    {
        return 0;
    }
    operation->registry = exports;
    *handle = (DVM_VfsHandle){.ops = &ops, .state = operation,
        .native_flags = DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_CLOSE};
    return 1;
}
