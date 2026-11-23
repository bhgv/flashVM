/* Copyright (c) 2026 bhgv
 * EN: Authenticate per-fid module channel operations and retire results by exact token.
 * RU: Проверка полномочий операций на fid и завершение выдач по точному токену.
 */
#include "styxrpc/module_channels.h"
#include <stdlib.h>
#include <string.h>

#define LIVE 1U      /* Token accepted for authentication. */
#define RETIRING 2U  /* No authentication or slot reuse until all closes finish. */
#define SERIAL_BYTES 4U /* Issuance number at the end of the opaque token. */

typedef struct StyxRpcModuleOpen
{
    StyxRpcModuleChannels *registry;   /* Present only before the first authentication attempt. */
    StyxRpcModuleChannel *lease;       /* Null until authenticated and after retirement. */
    void *cookie;                     /* Application handle owned while lease is nonnull. */
    struct StyxRpcModuleOpen *next;    /* Next operation on the same lease. */
    struct StyxRpcModuleOpen **link;   /* Exact pointer referencing this operation. */
} ModuleOpen;


/* EN: Look up a live token in this exact module registry.
 * RU: Найти действующий токен в данном реестре модуля. */
static StyxRpcModuleChannel *find_lease(StyxRpcModuleChannels *channels,
    const unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE])
{
    if (channels && channels->slots && token)
    {
        for (size_t i = 0; i < channels->capacity; ++i)
        {
            StyxRpcModuleChannel *slot = &channels->slots[i];
            unsigned difference = 0;
            for (size_t j = 0; j < sizeof(slot->token); ++j)
            {
                difference |= slot->token[j] ^ token[j];
            }
            if (slot->state == LIVE && !difference)
            {
                return slot;
            }
        }
    }
    return NULL;
}


/* EN/RU: See module_channels.h. / См. module_channels.h. */
int styxrpc_module_channels_init(StyxRpcModuleChannels *channels, StyxRpcModuleChannel *slots,
    size_t capacity, int (*entropy)(void *, void *, size_t), void *context)
{
    if (!channels || !slots || !capacity || capacity > SIZE_MAX / sizeof(*slots) || !entropy)
    {
        return 0;
    }
    memset(slots, 0, capacity * sizeof(*slots));
    *channels = (StyxRpcModuleChannels){.slots = slots, .capacity = capacity,
        .entropy = entropy, .entropy_context = context};
    return 1;
}


/* EN/RU: See module_channels.h. / См. module_channels.h. */
int styxrpc_module_channels_publish(StyxRpcModuleChannels *channels, const StyxRpcResource *resource,
    unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE])
{
    if (!channels || !channels->slots || !token || !resource || !resource->read || !resource->write ||
        channels->serial == UINT32_MAX)
    {
        return 0;
    }
    const size_t prefix = STYXRPC_CHANNEL_TOKEN_SIZE - SERIAL_BYTES;
    for (size_t i = 0; i < channels->capacity; ++i)
    {
        StyxRpcModuleChannel *slot = &channels->slots[i];
        if (slot->state)
        {
            continue;
        }
        unsigned char bytes[STYXRPC_CHANNEL_TOKEN_SIZE];
        if (channels->entropy(channels->entropy_context, bytes, prefix) != 1)
        {
            return 0;
        }
        uint32_t serial = ++channels->serial;
        for (unsigned j = 0; j < SERIAL_BYTES; ++j)
        {
            bytes[prefix + j] = (unsigned char)(serial >> (8 * (SERIAL_BYTES - j - 1)));
        }
        slot->resource = *resource;
        memcpy(slot->token, bytes, sizeof(bytes));
        slot->state = LIVE;
        memcpy(token, bytes, sizeof(bytes));
        return 1;
    }
    return 0;
}


/* EN: Detach an authenticated fid and release its application handle once.
 * RU: Отсоединить проверенный fid и единожды освободить его состояние приложения. */
static void retire(ModuleOpen *file)
{
    StyxRpcModuleChannel *slot = file->lease;
    if (!slot)
    {
        return;
    }
    *file->link = file->next;
    if (file->next)
    {
        file->next->link = file->link;
    }
    file->lease = NULL;
    if (slot->resource.close)
    {
        slot->resource.close(file->cookie);
    }
}


/* EN/RU: See module_channels.h. / См. module_channels.h. */
int styxrpc_module_channels_revoke(StyxRpcModuleChannels *channels,
    const unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE])
{
    StyxRpcModuleChannel *slot = find_lease(channels, token);
    if (!slot)
    {
        return 0;
    }
    slot->state = RETIRING;
    while (slot->opens)
    {
        retire(slot->opens);
    }
    *slot = (StyxRpcModuleChannel){0};
    return 1;
}


/* EN: Reserve a bus handle for exact read/write access. No lease is selected yet.
 * RU: Выделить дескриптор шины с доступом на чтение и запись. Выдача ещё не выбрана. */
static int open_bus(void *context, unsigned access, void **cookie)
{
    StyxRpcModuleChannels *channels = context;
    if (!channels || !channels->slots || access != (STYXRPC_READ | STYXRPC_WRITE))
    {
        return 0;
    }
    ModuleOpen *file = calloc(1, sizeof(*file));
    if (!file)
    {
        return 0;
    }
    file->registry = channels;
    *cookie = file;
    return 1;
}


/* EN: Bind the first complete token write to one active lease, with one attempt per fid.
 * RU: Привязать первую полную запись токена к действующей выдаче. Одна попытка на fid. */
static int authenticate(ModuleOpen *file, uint64_t offset, const void *bytes, size_t count)
{
    StyxRpcModuleChannels *channels = file->registry;
    file->registry = NULL;
    StyxRpcModuleChannel *slot = offset == 0 && count == STYXRPC_CHANNEL_TOKEN_SIZE ?
        find_lease(channels, bytes) : NULL;
    if (!slot)
    {
        return 0;
    }
    file->cookie = slot->resource.context;
    if (slot->resource.open && slot->resource.open(slot->resource.context,
        STYXRPC_READ | STYXRPC_WRITE, &file->cookie) != 1)
    {
        return 0;
    }
    file->lease = slot;
    file->next = slot->opens;
    file->link = &slot->opens;
    if (file->next)
    {
        file->next->link = &file->next;
    }
    slot->opens = file;
    return 1;
}


/* EN: Read the authorized resource's reply at its original positional offset.
 * RU: Читать ответ разрешённого ресурса с исходным смещением. */
static int read_bus(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    ModuleOpen *file = cookie;
    return file->lease && file->lease->resource.read(file->cookie, offset, bytes, capacity, size);
}


/* EN: Strip the token prefix from write offsets after authenticating this exact fid.
 * RU: После проверки данного fid вычесть длину токена из смещений записи. */
static int write_bus(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    ModuleOpen *file = cookie;
    if (file->registry)
    {
        if (!authenticate(file, offset, bytes, count))
        {
            return 0;
        }
        *size = count;
        return 1;
    }
    return file->lease && offset >= STYXRPC_CHANNEL_TOKEN_SIZE &&
        file->lease->resource.write(file->cookie, offset - STYXRPC_CHANNEL_TOKEN_SIZE, bytes, count, size);
}


/* EN: Release one bus fid, including unauthenticated or previously revoked fids.
 * RU: Закрыть fid шины, включая непроверенные и уже отозванные fid. */
static void close_bus(void *cookie)
{
    ModuleOpen *file = cookie;
    retire(file);
    free(file);
}


/* EN/RU: See module_channels.h. / См. module_channels.h. */
StyxRpcResource styxrpc_module_channels_resource(StyxRpcModuleChannels *channels)
{
    return (StyxRpcResource){channels, open_bus, read_bus, write_bus, close_bus};
}
