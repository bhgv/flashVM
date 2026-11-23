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
#include "root_registry.h"
#include "result_channels.h"
#include "program_view.h"
#include "native_wait.h"
#include "runtime_gc.h"
#include "disvm/refcount.h"
#include <string.h>
#include <stdlib.h>

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
    if (!dvm_runtime_root_add(heap))
    {
        return 0;
    }
    if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)
    {
        ptrdel(heap);
        return 0;
    }
    Setmark(heap);
    return 1;
}


/* [GC:] EN: Create or retain the canonical proxy of schema within a live lease.
 * RU: Создать или удержать канонический прокси схемы schema в живой аренде. */
static Channel *lease_proxy(DVM_ReturnedLease *lease, const DVM_ChannelSchema *schema)
{
    Channel *candidate = cnewc(&Tword, proxy_mover, 0);
    Channel *proxy = dvm_returned_proxy_arm(candidate, lease, schema) ?
        dvm_returned_proxy_canonical(candidate) : NULL;
    if (proxy != candidate)
    {
        destroy(candidate);
    }
    return proxy;
}


/* EN: Read a four-byte key in network order. data must span four bytes.
 * RU: Прочитать четырёхбайтовый ключ в сетевом порядке. В data должно быть четыре байта. */
static uint32_t read_key(const unsigned char *data)
{
    return (uint32_t)data[0] << 24 | (uint32_t)data[1] << 16 | (uint32_t)data[2] << 8 | data[3];
}


#include "channel_message_grants.inc"


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
void dvm_call_channels_release(DVM_CallChannels *channels)
{
    dvm_channel_route_release(channels->route);
    channels->route = NULL;
    dvm_call_channels_truncate(channels, 0);
    release_message_grants(channels);
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
void dvm_call_channels_truncate(DVM_CallChannels *channels, size_t first)
{
    while (channels->count > first)
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
static int write_tagged_key(unsigned long tag, uint32_t key, void *bytes, size_t capacity, size_t *size)
{
    unsigned char identity[4] = {key >> 24, key >> 16, key >> 8, key};
    return fvm_srecord_write(tag, identity, sizeof(identity),
        bytes, capacity, size) == FVM_S_OK;
}


/* EN: Write the ordinary key in a direction that already knows its channel.
 * RU: Записать обычный ключ в направлении, где канал уже известен. */
static int write_key(uint32_t key, void *bytes, size_t capacity, size_t *size)
{
    return write_tagged_key(STYX_MANAGED_CHANNEL_TAG, key, bytes, capacity, size);
}


/* EN: Add the key to the opaque provider result token.
 * RU: Добавить ключ к непрозрачному токену результата провайдера. */
static int write_export(const DVM_ChannelSchema *schema, void *bytes, size_t capacity, size_t *size)
{
    unsigned char identity[STYX_CHANNEL_EXPORT_TOKEN_SIZE + 4];
    if (!dvm_result_channels_token(identity))
    {
        return 0;
    }
    uint32_t key = schema->key | (schema->unbuffered ? UINT32_C(0x80000000) : 0);
    identity[STYX_CHANNEL_EXPORT_TOKEN_SIZE] = key >> 24;
    identity[STYX_CHANNEL_EXPORT_TOKEN_SIZE + 1] = key >> 16;
    identity[STYX_CHANNEL_EXPORT_TOKEN_SIZE + 2] = key >> 8;
    identity[STYX_CHANNEL_EXPORT_TOKEN_SIZE + 3] = key;
    return fvm_srecord_write(STYX_MANAGED_EXPORTED_CHANNEL_TAG, identity, sizeof(identity),
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
    if (channels->persistent)
    {
        if (!dvm_call_channel_is_proxy(channel) ||
            dvm_returned_proxy_lease(channel) != channels->persistent)
        {
            uint32_t pid, key;
            return dvm_returned_lease_grant(channels->persistent, channel, type, &pid, &key) &&
                write_grant(pid, key, bytes, capacity, size);
        }
        DVM_Provider *endpoint;
        DVM_Cancel *scope;
        const unsigned char *token;
        const DVM_ChannelSchema *original = dvm_returned_proxy_route(channel, &endpoint, &scope, &token);
        return original && !strcmp(original->shape, schema.shape) &&
            write_key(original->key, bytes, capacity, size);
    }
    const MessageImport *imported = message_import(channels, channel);
    if (imported)
    {
        const DVM_ChannelSchema *original = &imported->schema;
        uint32_t key = original->key | (original->unbuffered ? UINT32_C(0x80000000) : 0);
        return !strcmp(original->shape, schema.shape) &&
            write_grant(imported->pid, key, bytes, capacity, size);
    }
    if (channels->message || (channels->returning && dvm_call_channel_is_proxy(channel)))
    {
        if (!dvm_call_channel_is_proxy(channel) || channel->aux)
        {
            return 0;
        }
        uint32_t token = (uint32_t)channel->mid.w;
        DVM_Provider *provider;
        DVM_Cancel *scope = NULL;
        const DVM_ChannelSchema *original = channels->message ?
            dvm_channel_route_lookup(token, &provider, &scope) : dvm_channel_route_result(token);
        return original && !strcmp(original->shape, schema.shape) &&
            (!channels->message || scope == channels->message) &&
            write_key(original->key, bytes, capacity, size);
    }
    if (dvm_call_channel_is_proxy(channel))
    {
        DVM_Provider *endpoint;
        DVM_Cancel *scope;
        const unsigned char *token;
        const DVM_ChannelSchema *original = dvm_returned_proxy_route(channel, &endpoint, &scope, &token);
        if (!original || strcmp(original->shape, schema.shape))
        {
            return 0;
        }
        schema.unbuffered = original->unbuffered;
    }
    else if (!dvm_channel_value_mover(channel, schema.shape))
    {
        return 0;
    }
    else
    {
        schema.unbuffered = channel->buf == H;
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
    const DVM_ChannelSchema *issued = &channels->schemas[slot];
    if (channels->returning)
    {
        return write_export(issued, bytes, capacity, size);
    }
    unsigned long tag = channels->message_owner && issued->unbuffered ?
        STYX_MANAGED_UNBUFFERED_CHANNEL_TAG : STYX_MANAGED_CHANNEL_TAG;
    return write_tagged_key(tag, issued->key, bytes, capacity, size);
}


/* [GC:] EN: Decode one provider-owned result leaf, sharing a group pin and aliases.
 * RU: Декодировать полномочие провайдера с общей арендой и алиасами результата. */
static int decode_export(DVM_ReturnedImport *imports, const char *type,
    const FVM_SRecord *record, void **result)
{
    if (!imports || record->size != STYX_CHANNEL_EXPORT_TOKEN_SIZE + 4 ||
        !dvm_returned_lease_token(imports->lease, record->payload))
    {
        return 0;
    }
    uint32_t wire = read_key(record->payload + STYX_CHANNEL_EXPORT_TOKEN_SIZE);
    DVM_ChannelSchema schema = {.key = wire & INT32_MAX, .unbuffered = wire >> 31};
    if (!schema.key || schema.key > INT32_MAX || !dvm_channel_value_shape(type, schema.shape))
    {
        return 0;
    }
    for (size_t i = 0; i < imports->count; i++)
    {
        if (imports->schemas[i].key == schema.key)
        {
            Channel *proxy = imports->objects[i];
            if (imports->schemas[i].unbuffered != schema.unbuffered ||
                strcmp(imports->schemas[i].shape, schema.shape) ||
                dvm_refcount_retain_u(&D2H(proxy)->ref) != DVM_REFCOUNT_LIVE)
            {
                return 0;
            }
            *result = proxy;
            return 1;
        }
    }
    if (imports->count == DVM_CHANNEL_BUS_SLOTS)
    {
        return 0;
    }
    Channel *proxy = lease_proxy(imports->lease, &schema);
    if (!proxy || !root_channel(proxy))
    {
        if (proxy)
        {
            destroy(proxy);
        }
        return 0;
    }
    size_t slot = imports->count++;
    imports->objects[slot] = proxy;
    imports->schemas[slot] = schema;
    *result = proxy;
    return 1;
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
    if (channels && record->tag == STYX_MANAGED_CHANNEL_GRANT_TAG)
    {
        return decode_grant(channels, type, record, result);
    }
    if (record->tag == STYX_MANAGED_EXPORTED_CHANNEL_TAG)
    {
        return channels && channels->resolving && decode_export(channels->imports, type, record, result);
    }
    int unbuffered = channels && channels->persistent && record->tag == STYX_MANAGED_UNBUFFERED_CHANNEL_TAG;
    if (!channels || (record->tag != STYX_MANAGED_CHANNEL_TAG && !unbuffered) || record->size != 4)
    {
        return 0;
    }
    uint32_t key = read_key(record->payload);
    DVM_ChannelSchema schema = {.key = key, .unbuffered = unbuffered};
    if (!key || key > INT32_MAX || !dvm_channel_value_shape(type, schema.shape))
    {
        return 0;
    }
    if (channels->persistent)
    {
        Channel *proxy = lease_proxy(channels->persistent, &schema);
        if (!proxy)
        {
            return 0;
        }
        *result = proxy;
        return 1;
    }
    if (channels->resolving)
    {
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
        return 0;
    }
    if (!channels->message && !channels->route)
    {
        channels->route = dvm_channel_route_create();
    }
    DVM_ChannelRoute *route = channels->message ?
        dvm_channel_route_for_message(channels->message) : channels->route;
    if (!route)
    {
        return 0;
    }
    Channel *proxy = dvm_channel_route_proxy(route, &schema, NULL);
    if (proxy)
    {
        *result = proxy;
        return 1;
    }
    uint32_t token = allocate_key();
    if (!token)
    {
        return 0;
    }
    Channel *candidate = cnewc(&Tword, proxy_mover, 0);
    candidate->mid.w = token;
    if (dvm_runtime_root_add(D2H(candidate)))
    {
        Setmark(D2H(candidate));
        proxy = dvm_channel_route_proxy(route, &schema, candidate);
        ptrdel(D2H(candidate));
    }
    destroy(candidate);
    if (!proxy)
    {
        return 0;
    }
    *result = proxy;
    return 1;
}
