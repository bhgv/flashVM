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
    DVM_ChannelSchema schemas[DVM_CHANNEL_BUS_SLOTS]; /* Types tied to the exact capability. */
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
    size_t staged; /* Incoming bytes buffered before complete admission. */
    unsigned char reply[DVM_CHANNEL_BUS_WIRE]; /* Complete wire reply. */
    size_t size; /* Zero until completion. */
    size_t read; /* Number of reply bytes already returned. */
    int rejected; /* Bad framing poisons this fid. */
};


/* EN/RU: See channel_bus.h. / См. channel_bus.h. */
int dvm_channel_bus_write(uint32_t key, int receive, const void *element, size_t extent,
    unsigned char *bytes, size_t capacity, size_t *size)
{
    unsigned char identity[4] = {key >> 24, key >> 16, key >> 8, key};
    return key && (receive ? extent == 0 : extent != 0) &&
        fvm_srecord_write_pair(receive ? DVM_CHANNEL_BUS_RECV : DVM_CHANNEL_BUS_SEND,
            identity, sizeof(identity), element, extent, bytes, capacity, size) == FVM_S_OK;
}


/* EN: Admit a nested capability only if this bus already issued its exact key and type.
 * RU: Допустить вложенное полномочие только после выдачи точного ключа и типа этой шиной. */
static int admit_nested(const void *context, const char *type, uint32_t key)
{
    const DVM_ChannelBus *bus = context;
    const char *end = styxshape_end(type);
    size_t size = (size_t)(end - type) - 3;
    for (size_t i = 0; i < bus->count; i++)
    {
        if (bus->schemas[i].key == key)
        {
            const char *shape = bus->schemas[i].shape;
            return strlen(shape) == size + 2 && !memcmp(shape + 1, type + 2, size);
        }
    }
    return 0;
}


/* EN: Parse one direction and check its key against immutable call keys.
 * RU: Разобрать одно направление и проверить его ключ по ключам вызова. */
static int parse_choice(const DVM_ChannelBus *bus, const FVM_SRecord *record,
    DVM_ChannelOperation *request)
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
    while (slot < bus->count && bus->schemas[slot].key != key)
    {
        slot++;
    }
    if (slot == bus->count)
    {
        return 0;
    }
    const char *shape = bus->schemas[slot].shape;
    int typed = strcmp(shape, "(i)") != 0;
    int receive = record->tag == DVM_CHANNEL_BUS_RECV;
    size_t size = record->size - 4;
    int64_t integer = 0;
    if (receive)
    {
        if (size)
        {
            return 0;
        }
    }
    else if (typed)
    {
        FVM_SRecord value;
        size_t used;
        if (fvm_srecord_read(id + 4, size, STYX_MANAGED_LIST_LIMIT, &value, &used) != FVM_S_OK ||
            used != size || !styxmanaged_channel_payload(shape, &value, admit_nested, bus))
        {
            return 0;
        }
    }
    else
    {
        StyxScalarKind kind;
        if (!styxscalarread(STYX_CALL_INT_TAG, id + 4, size, &kind, &integer) ||
            kind != STYX_SCALAR_VALUE || integer < INT32_MIN || integer > INT32_MAX)
        {
            return 0;
        }
    }
    request->typed |= typed;
    request->choices[request->count++] = (DVM_ChannelChoice){.slot = slot, .receive = receive,
        .offset = (size_t)(id + 4 - request->bytes), .size = size, .integer = (int32_t)integer};
    return 1;
}


/* EN: Parse one complete operation, including every guard before admission.
 * RU: Разобрать полную операцию и проверить все её ветки до приёма. */
static FVM_SResult parse_request(const DVM_ChannelBus *bus, size_t size, DVM_ChannelOperation *request)
{
    FVM_SRecord record;
    size_t used;
    FVM_SResult decoded = fvm_srecord_read(request->bytes, size, DVM_CHANNEL_BUS_WIRE, &record, &used);
    if (decoded != FVM_S_OK)
    {
        return decoded;
    }
    if (used != size)
    {
        return FVM_S_INVALID;
    }
    request->extent = size;
    if (record.tag != DVM_CHANNEL_BUS_ALT && record.tag != DVM_CHANNEL_BUS_TRY_ALT)
    {
        return parse_choice(bus, &record, request) ? FVM_S_OK : FVM_S_INVALID;
    }
    request->alt = 1;
    request->immediate = record.tag == DVM_CHANNEL_BUS_TRY_ALT;
    const unsigned char *cursor = record.payload;
    size_t remaining = record.size;
    while (remaining)
    {
        if (request->count == DVM_CHANNEL_BUS_SLOTS ||
            fvm_srecord_read(cursor, remaining, DVM_CHANNEL_BUS_WIRE, &record, &used) != FVM_S_OK ||
            !parse_choice(bus, &record, request))
        {
            return FVM_S_INVALID;
        }
        cursor += used;
        remaining -= used;
    }
    return request->count ? FVM_S_OK : FVM_S_INVALID;
}


/* EN: Stage one request per open fid. A refusal leaves the cursor untouched.
 * RU: Поставить один запрос на открытый fid. Отказ не меняет курсор. */
static void write_bus(void *state, const void *bytes, size_t size, DVM_IoResult *out)
{
    BusFid *fid = state;
    DVM_ChannelBus *bus = fid->bus;
    *out = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
    fvm_port_sync_lock(bus->lock);
    if (!bus->sealed && !fid->rejected && !fid->request.sequence && bus->sequence != UINT64_MAX &&
        size && size <= DVM_CHANNEL_BUS_WIRE - fid->staged &&
        !dvm_cancel_requested(dvm_cancel_current()))
    {
        memcpy(fid->request.bytes + fid->staged, bytes, size);
        fid->staged += size;
        FVM_SResult decoded = parse_request(bus, fid->staged, &fid->request);
        if (decoded == FVM_S_OK)
        {
            fid->request.sequence = ++bus->sequence;
        }
        else
        {
            fid->rejected = decoded != FVM_S_MORE || fid->staged == DVM_CHANNEL_BUS_WIRE;
        }
        if (!fid->rejected)
        {
            *out = (DVM_IoResult){.status = FVM_OK, .count = size, .flags = DVM_IO_PREFIX};
        }
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
    else if (fid->rejected)
    {
        out->status = FVM_ERROR;
        out->stop = DVM_IO_DENIED;
    }
    else if (fid->size && fid->read == fid->size)
    {
        out->stop = DVM_IO_EOF;
    }
    else if (fid->size && size)
    {
        size_t left = fid->size - fid->read;
        out->count = size < left ? size : left;
        memcpy(bytes, fid->reply + fid->read, out->count);
        fid->read += out->count;
        if (fid->read == fid->size)
        {
            out->flags |= DVM_IO_RECORD;
        }
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
DVM_Provider *dvm_channel_bus_create(const DVM_ChannelSchema *schemas, size_t count, DVM_ChannelBus **out)
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
    memcpy(bus->schemas, schemas, count * sizeof(*schemas));
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
int dvm_channel_bus_extend(DVM_ChannelBus *bus, const DVM_ChannelSchema *schemas, size_t count)
{
    fvm_port_sync_lock(bus->lock);
    int valid = !bus->sealed && count <= DVM_CHANNEL_BUS_SLOTS - bus->count;
    if (valid)
    {
        memcpy(bus->schemas + bus->count, schemas, count * sizeof(*schemas));
        bus->count += count;
    }
    fvm_port_sync_unlock(bus->lock);
    return valid;
}


/* EN/RU: See channel_bus.h. / См. channel_bus.h. */
int dvm_channel_bus_next(DVM_ChannelBus *bus, uint64_t after, DVM_ChannelOperation *operation)
{
    uint64_t selected = 0;
    BusFid *pending = NULL;
    fvm_port_sync_lock(bus->lock);
    for (BusFid *fid = bus->sealed ? NULL : bus->head; fid; fid = fid->next)
    {
        uint64_t sequence = fid->request.sequence;
        if (sequence > after && (!selected || sequence < selected) && !fid->size)
        {
            selected = sequence;
            pending = fid;
        }
    }
    if (pending)
    {
        memcpy(operation, &pending->request, offsetof(DVM_ChannelOperation, bytes) + pending->request.extent);
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


#include "channel_bus_pair.inc"


/* EN/RU: See channel_bus.h. / См. channel_bus.h. */
void dvm_channel_bus_finish(DVM_ChannelBus *bus, const DVM_ChannelOperation *operation,
    int success, int64_t value, unsigned selected, const void *element, size_t size)
{
    fvm_port_sync_lock(bus->lock);
    for (BusFid *fid = bus->sealed ? NULL : bus->head; fid; fid = fid->next)
    {
        if (fid->request.sequence == operation->sequence && !fid->size)
        {
            fid->size = stage_bus_reply(fid, success, value, selected, element, size);
            if (!fid->size)
            {
                fid->size = stage_bus_reply(fid, 0, 0, 0, NULL, 0);
            }
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
