/* Copyright (c) 2026 bhgv
 * EN: Bound and route UniBus channel messages by capability and open Styx fid.
 * RU: Ограничение и маршрутизация сообщений UniBus по полномочию и открытому fid Styx.
 */
#include "channel_bus.h"
#include "cancel.h"
#include "channel_sync.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/scalar_record.h"
#include <stdlib.h>
#include <string.h>

typedef struct BusFid BusFid; /* One open protocol transaction. */


/* EN: All shared state is native. The lock orders admission, close and replies.
 * RU: Общее состояние нативное. Мьютекс упорядочивает приём, закрытие и ответы. */
struct DVM_ChannelBus
{
    FVM_PortSync *lock; /* Exclusive access to fids and lifecycle state. */
    BusFid *head; /* Borrowed live fids, newest first. */
    uint64_t sequence; /* Last assigned request sequence. */
    uint32_t keys[DVM_CHANNEL_BUS_SLOTS]; /* Capability identities for this call. */
    size_t count; /* Active capability count. */
    unsigned open; /* Bounded concurrent fids, including idle ones. */
    int sealed; /* Caller has retired or cancelled this route. */
};


/* EN: Provider ownership keeps bus alive until this cursor closes.
 * RU: Ссылка провайдера удерживает bus до закрытия курсора. */
struct BusFid
{
    DVM_ChannelBus *bus; /* Borrowed from the retained provider. */
    BusFid *next; /* Borrowed sibling, protected by lock. */
    DVM_ChannelOperation request; /* Sequence zero before the first accepted write. */
    unsigned char reply[DVM_CHANNEL_BUS_WIRE]; /* Complete wire reply. */
    size_t size; /* Zero until completion. */
    int read; /* The sole reply has already been read. */
};


/* EN/RU: See channel_bus.h. / См. channel_bus.h. */
int dvm_channel_bus_write(uint32_t key, int receive, int32_t value,
    unsigned char *bytes, size_t capacity, size_t *size)
{
    unsigned char identity[4] = {key >> 24, key >> 16, key >> 8, key};
    unsigned char payload[STYX_SCALAR_RECORD_CAPACITY];
    size_t length = 0;
    return key && (receive || styxscalarwrite(STYX_CALL_INT_TAG, STYX_SCALAR_VALUE,
        value, payload, sizeof(payload), &length)) &&
        fvm_srecord_write_pair(receive ? DVM_CHANNEL_BUS_RECV : DVM_CHANNEL_BUS_SEND,
            identity, sizeof(identity), payload, length, bytes, capacity, size) == FVM_S_OK;
}


/* EN: Parse one direction and check its key against immutable call keys.
 * RU: Разобрать одно направление и проверить его ключ по ключам вызова. */
static int parse_choice(const DVM_ChannelBus *bus, const FVM_SRecord *record,
    DVM_ChannelChoice *choice)
{
    if (record->size < 4 ||
        (record->tag != DVM_CHANNEL_BUS_SEND && record->tag != DVM_CHANNEL_BUS_RECV))
    {
        return 0;
    }
    const unsigned char *id = record->payload;
    const uint32_t key = (uint32_t)id[0] << 24 | (uint32_t)id[1] << 16 |
        (uint32_t)id[2] << 8 | id[3];
    unsigned slot = 0;
    while (slot < bus->count && bus->keys[slot] != key)
    {
        slot++;
    }
    if (slot == bus->count)
    {
        return 0;
    }
    int64_t value = 0;
    int receive = record->tag == DVM_CHANNEL_BUS_RECV;
    StyxScalarKind kind;
    if (receive ? record->size != 4 :
        (!styxscalarread(STYX_CALL_INT_TAG, id + 4, record->size - 4, &kind, &value) ||
        kind != STYX_SCALAR_VALUE || value < INT32_MIN || value > INT32_MAX))
    {
        return 0;
    }
    *choice = (DVM_ChannelChoice){.slot = slot, .receive = receive, .value = (int32_t)value};
    return 1;
}


/* EN: Parse one complete operation, including every guard before admission.
 * RU: Разобрать полную операцию и проверить все её ветки до приёма. */
static int parse_request(const DVM_ChannelBus *bus, const void *bytes, size_t size,
    DVM_ChannelOperation *request)
{
    FVM_SRecord record;
    size_t used;
    if (size > DVM_CHANNEL_BUS_WIRE ||
        fvm_srecord_read(bytes, size, DVM_CHANNEL_BUS_WIRE, &record, &used) != FVM_S_OK ||
        used != size)
    {
        return 0;
    }
    *request = (DVM_ChannelOperation){0};
    if (record.tag != DVM_CHANNEL_BUS_ALT && record.tag != DVM_CHANNEL_BUS_TRY_ALT)
    {
        request->count = 1;
        return parse_choice(bus, &record, &request->choices[0]);
    }
    request->alt = 1;
    request->immediate = record.tag == DVM_CHANNEL_BUS_TRY_ALT;
    const unsigned char *cursor = record.payload;
    size_t remaining = record.size;
    while (remaining)
    {
        if (request->count == DVM_CHANNEL_BUS_SLOTS ||
            fvm_srecord_read(cursor, remaining, DVM_CHANNEL_BUS_WIRE, &record, &used) != FVM_S_OK ||
            !parse_choice(bus, &record, &request->choices[request->count]))
        {
            return 0;
        }
        request->count++;
        cursor += used;
        remaining -= used;
    }
    return request->count != 0;
}


/* EN: Stage one request per open fid. A refusal leaves the cursor untouched.
 * RU: Поставить один запрос на открытый fid. Отказ не меняет курсор. */
static void write_bus(void *state, const void *bytes, size_t size, DVM_IoResult *out)
{
    BusFid *fid = state;
    DVM_ChannelBus *bus = fid->bus;
    DVM_ChannelOperation request;
    *out = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
    if (!parse_request(bus, bytes, size, &request))
    {
        return;
    }
    fvm_port_sync_lock(bus->lock);
    if (!bus->sealed && !fid->request.sequence && bus->sequence != UINT64_MAX &&
        !dvm_cancel_requested(dvm_cancel_current()))
    {
        request.sequence = ++bus->sequence;
        fid->request = request;
        *out = (DVM_IoResult){.status = FVM_OK, .count = size, .flags = DVM_IO_PREFIX};
    }
    fvm_port_sync_unlock(bus->lock);
}


/* EN: Return one complete reply; zero while waiting, error after retirement.
 * RU: Вернуть полный ответ, ноль в ожидании либо ошибку после отзыва. */
static void read_bus(void *state, void *bytes, size_t size, DVM_IoResult *out)
{
    BusFid *fid = state;
    DVM_ChannelBus *bus = fid->bus;
    *out = (DVM_IoResult){.status = FVM_OK, .flags = DVM_IO_PREFIX};
    fvm_port_sync_lock(bus->lock);
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        out->status = FVM_ERROR;
        out->stop = DVM_IO_CANCELLED;
    }
    else if (bus->sealed)
    {
        out->status = FVM_ERROR;
        out->stop = DVM_IO_CANCELLED;
    }
    else if (fid->read)
    {
        out->stop = DVM_IO_EOF;
    }
    else if (fid->size && size >= fid->size)
    {
        memcpy(bytes, fid->reply, fid->size);
        out->count = fid->size;
        out->flags |= DVM_IO_RECORD;
        fid->read = 1;
    }
    else if (fid->size && size)
    {
        out->status = FVM_ERROR;
        out->stop = DVM_IO_DENIED;
    }
    fvm_port_sync_unlock(bus->lock);
}


/* EN: Drop one exact fid and remove its unpublished operation if present.
 * RU: Снять точный fid вместе с его ожидающей операцией, если она есть. */
static void close_bus(void *state)
{
    BusFid *fid = state;
    DVM_ChannelBus *bus = fid->bus;
    fvm_port_sync_lock(bus->lock);
    BusFid **entry = &bus->head;
    while (*entry != fid)
    {
        entry = &(*entry)->next;
    }
    *entry = fid->next;
    bus->open--;
    fvm_port_sync_unlock(bus->lock);
    free(fid);
}


/* EN: Describe the only node of this native provider.
 * RU: Описать единственный узел этого нативного провайдера. */
static FVM_Result bus_info(void *state, const char *path, FVM_FileInfo *info)
{
    (void)state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(info, 0, sizeof(*info));
    strcpy(info->name, ".unibus");
    info->mode = 0666;
    return FVM_OK;
}


/* EN: Open one independent request/reply cursor on the borrowed live provider.
 * RU: Открыть независимый курсор запроса и ответа на заимствованном провайдере. */
static FVM_Result open_bus(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    static const DVM_VfsOps ops = {.read_chunk = read_bus, .write_chunk = write_bus,
        .close = close_bus}; /* Whole records, no seeking. */
    DVM_ChannelBus *bus = state;
    if (strcmp(path, ".") || strcmp(mode, "rw"))
    {
        return FVM_ERROR;
    }
    BusFid *fid = calloc(1, sizeof(*fid));
    if (!fid)
    {
        return FVM_OUT_OF_MEMORY;
    }
    fvm_port_sync_lock(bus->lock);
    int admitted = !bus->sealed && bus->open < DVM_CHANNEL_BUS_SLOTS * 2;
    if (admitted)
    {
        fid->bus = bus;
        fid->next = bus->head;
        bus->head = fid;
        bus->open++;
    }
    fvm_port_sync_unlock(bus->lock);
    if (!admitted)
    {
        free(fid);
        return FVM_ERROR;
    }
    *out = (DVM_VfsHandle){.state = fid, .ops = &ops,
        .native_flags = DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_CLOSE};
    return FVM_OK;
}


/* EN: Release the bus after the provider and all its opened fids are gone.
 * RU: Освободить bus после провайдера и всех открытых fid. */
static void destroy_bus(void *state)
{
    DVM_ChannelBus *bus = state;
    fvm_port_sync_destroy(bus->lock);
    free(bus);
}


/* EN/RU: See channel_bus.h. / См. channel_bus.h. */
DVM_Provider *dvm_channel_bus_create(const uint32_t *keys, size_t count, DVM_ChannelBus **out)
{
    static const DVM_ProviderOps ops = {.info = bus_info, .open = open_bus,
        .destroy = destroy_bus, .flags = DVM_PROVIDER_STABLE_BIND}; /* Virtual .unibus file. */
    *out = NULL;
    DVM_ChannelBus *bus = count && count <= DVM_CHANNEL_BUS_SLOTS ? calloc(1, sizeof(*bus)) : NULL;
    if (!bus || !(bus->lock = fvm_port_sync_create()))
    {
        free(bus);
        return NULL;
    }
    memcpy(bus->keys, keys, count * sizeof(*keys));
    bus->count = count;
    DVM_Provider *provider = dvm_provider_create_native(&ops, bus, NULL, 1,
        DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_CAPTURE);
    if (!provider)
    {
        destroy_bus(bus);
        return NULL;
    }
    *out = bus;
    return provider;
}


/* EN/RU: See channel_bus.h. / См. channel_bus.h. */
int dvm_channel_bus_next(DVM_ChannelBus *bus, uint64_t after, DVM_ChannelOperation *operation)
{
    uint64_t selected = 0;
    fvm_port_sync_lock(bus->lock);
    for (BusFid *fid = bus->sealed ? NULL : bus->head; fid; fid = fid->next)
    {
        uint64_t sequence = fid->request.sequence;
        if (sequence > after && (!selected || sequence < selected) && !fid->size)
        {
            selected = sequence;
            *operation = fid->request;
        }
    }
    fvm_port_sync_unlock(bus->lock);
    return selected != 0;
}


/* EN/RU: See channel_bus.h. / См. channel_bus.h. */
int dvm_channel_bus_pending(DVM_ChannelBus *bus, uint64_t sequence)
{
    fvm_port_sync_lock(bus->lock);
    int pending = 0;
    for (BusFid *fid = bus->sealed ? NULL : bus->head; fid; fid = fid->next)
    {
        if (fid->request.sequence == sequence && !fid->size)
        {
            pending = 1;
            break;
        }
    }
    fvm_port_sync_unlock(bus->lock);
    return pending;
}


/* EN/RU: See channel_bus.h. / См. channel_bus.h. */
void dvm_channel_bus_finish(DVM_ChannelBus *bus, const DVM_ChannelOperation *operation,
    int success, int64_t value)
{
    fvm_port_sync_lock(bus->lock);
    for (BusFid *fid = bus->sealed ? NULL : bus->head; fid; fid = fid->next)
    {
        if (fid->request.sequence == operation->sequence && !fid->size)
        {
            styxscalarwrite(DVM_CHANNEL_BUS_REPLY, success ? STYX_SCALAR_VALUE : STYX_SCALAR_ERROR,
                value, fid->reply, sizeof(fid->reply), &fid->size);
            break;
        }
    }
    fvm_port_sync_unlock(bus->lock);
}


/* EN/RU: See channel_bus.h. / См. channel_bus.h. */
void dvm_channel_bus_seal(DVM_ChannelBus *bus)
{
    if (bus)
    {
        fvm_port_sync_lock(bus->lock);
        bus->sealed = 1;
        fvm_port_sync_unlock(bus->lock);
    }
}
