/* Copyright (c) 2026 bhgv
 * EN: Export local channels and construct scoped proxies for matched argument identities.
 * RU: Выдача локальных каналов и создание прокси для совпадающих ключей аргументов.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "channel_route.h"
#include "channel_value.h"
#include "disvm/refcount.h"
#include <string.h>

static uint32_t next_key; /* Never reissue a capability in this process. */



/* EN: Reserved mover identity. Ordinary queue transfers cannot use a proxy.
 * RU: Маркер мувера. Обычные операции с очередью не используют прокси. */
static void proxy_mover(void)
{
    error("remote channel requires a UniBus operation");
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
int dvm_call_channel_is_proxy(Channel *channel)
{
    return channel && channel != H && channel->mover == proxy_mover;
}


/* [GC:] EN: Retain one live heap channel before inserting it in an owner table.
 * RU: Удержать живой канал перед добавлением в таблицу владельца. */
static int root_channel(Channel *channel)
{
    Heap *heap = D2H(channel);
    if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)
    {
        return 0;
    }
    ptradd(heap);
    Setmark(heap);
    return 1;
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
void dvm_call_channels_release(DVM_CallChannels *channels)
{
    dvm_channel_route_release(channels->route);
    channels->route = NULL;
    while (channels->count)
    {
        Channel *object = channels->objects[--channels->count];
        ptrdel(D2H(object));
        destroy(object);
    }
}


/* EN: Allocate a positive wire identity without wraparound.
 * RU: Выделить положительный wire-идентификатор без переполнения. */
static uint32_t allocate_key(void)
{
    uint32_t key = __atomic_load_n(&next_key, __ATOMIC_RELAXED);
    while (key < INT32_MAX)
    {
        if (__atomic_compare_exchange_n(&next_key, &key, key + 1, 1, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            return key + 1;
        }
    }
    return 0;
}


/* EN: Write the same positive identifier in requests and returned aliases.
 * RU: Записать одинаковый положительный идентификатор в запросе и возврате. */
static int write_key(uint32_t key, void *bytes, size_t capacity, size_t *size)
{
    unsigned char identity[4] = {key >> 24, key >> 16, key >> 8, key};
    return fvm_srecord_write(STYX_MANAGED_CHANNEL_TAG, identity, sizeof(identity),
        bytes, capacity, size) == FVM_S_OK;
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
int dvm_call_channel_encode(DVM_CallChannels *channels, const char *type, void *value,
    void *bytes, size_t capacity, size_t *size)
{
    if (value == H)
    {
        return fvm_srecord_write(STYX_MANAGED_NIL_TAG, NULL, 0, bytes, capacity, size) == FVM_S_OK;
    }
    Channel *channel = value;
    DVM_ChannelSchema schema = {0};
    if (!channels || !dvm_channel_value_shape(type, schema.shape))
    {
        return 0;
    }
    if (channels->returning)
    {
        const DVM_ChannelSchema *original = dvm_call_channel_is_proxy(channel) ?
            dvm_channel_route_result((uint32_t)channel->mid.w) : NULL;
        return original && !strcmp(original->shape, schema.shape) &&
            write_key(original->key, bytes, capacity, size);
    }
    if (!dvm_channel_value_mover(channel, schema.shape))
    {
        return 0;
    }
    size_t slot = 0;
    while (slot < channels->count && channels->objects[slot] != channel)
    {
        slot++;
    }
    if (slot == channels->count)
    {
        uint32_t key = slot < DVM_CHANNEL_BUS_SLOTS ? allocate_key() : 0;
        if (!key || !root_channel(channel))
        {
            return 0;
        }
        channels->objects[slot] = channel;
        schema.key = key;
        channels->schemas[slot] = schema;
        channels->count++;
    }
    if (strcmp(channels->schemas[slot].shape, schema.shape))
    {
        return 0;
    }
    return write_key(channels->schemas[slot].key, bytes, capacity, size);
}


/* [GC:] EN: Build one local proxy; repeated keys return the same channel.
 * RU: Создать локальный прокси. Повторные ключи возвращают тот же канал. */
int dvm_call_channel_decode(DVM_CallChannels *channels, const char *type,
    const FVM_SRecord *record, void **result)
{
    if (record->tag == STYX_MANAGED_NIL_TAG && !record->size)
    {
        *result = H;
        return 1;
    }
    if (!channels || record->tag != STYX_MANAGED_CHANNEL_TAG || record->size != 4)
    {
        return 0;
    }
    const unsigned char *id = record->payload;
    uint32_t key = (uint32_t)id[0] << 24 | (uint32_t)id[1] << 16 | (uint32_t)id[2] << 8 | id[3];
    DVM_ChannelSchema schema = {.key = key};
    if (!key || key > INT32_MAX || !dvm_channel_value_shape(type, schema.shape))
    {
        return 0;
    }
    for (size_t i = 0; i < channels->count; i++)
    {
        if (channels->schemas[i].key == key)
        {
            Channel *alias = channels->objects[i];
            if (strcmp(channels->schemas[i].shape, schema.shape) ||
                dvm_refcount_retain_u(&D2H(alias)->ref) != DVM_REFCOUNT_LIVE)
            {
                return 0;
            }
            *result = alias;
            return 1;
        }
    }
    if (channels->resolving || channels->count == DVM_CHANNEL_BUS_SLOTS)
    {
        return 0;
    }
    if (!channels->route)
    {
        channels->route = dvm_channel_route_create();
    }
    uint32_t token = allocate_key();
    if (!dvm_channel_route_add(channels->route, token, &schema))
    {
        return 0;
    }
    Channel *proxy = cnewc(&Tword, proxy_mover, 0);
    proxy->mid.w = token;
    if (!root_channel(proxy))
    {
        destroy(proxy);
        return 0;
    }
    size_t slot = channels->count++;
    channels->objects[slot] = proxy;
    channels->schemas[slot] = schema;
    *result = proxy;
    return 1;
}
