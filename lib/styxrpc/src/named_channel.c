/* Copyright (c) 2026 bhgv
 * EN: Own named endpoints and pair pending sends and receives without a buffer.
 * RU: Владеть именованными участниками и сопоставлять запись и чтение без буфера.
 */
#include "styxrpc/named_channel.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct NamedChannel
{
    struct NamedChannel *next;        /* Next live name in the same registry. */
    StyxRpcNamedEndpoint *head;       /* Oldest unresolved operation. */
    StyxRpcNamedEndpoint *tail;       /* Newest unresolved operation. */
    uint64_t lease;                  /* Exact native name reservation. */
    unsigned participants;           /* Every enrolled endpoint owns one reference. */
    char path[STYXRPC_VFS_PATH_MAX];    /* Absolute NUL-terminated name. */
} NamedChannel;

struct StyxRpcNamedRegistry
{
    pthread_mutex_t lock;      /* Serializes pending request/reply state. */
    pthread_mutex_t names_lock; /* Serializes new paths, visibility checks and final release. */
    NamedChannel *names;   /* Owned live names. */
    StyxRpcNamedClaim claim; /* Reserves a new name in the ordinary namespace. */
    StyxRpcNamedVisible visible; /* Verifies the exact claim before another participant joins. */
    StyxRpcNamedDrop drop;   /* Removes the claim from the ordinary namespace. */
    void *state;             /* Borrowed by claim/drop until destroy. */
};

struct StyxRpcNamedEndpoint
{
    StyxRpcNamedRegistry *registry;  /* Borrowed from the native provider. */
    NamedChannel *channel;           /* One owned enrollment, NULL until open. */
    StyxRpcNamedEndpoint *next;      /* Pending FIFO only. */
    unsigned char *body;             /* Owned staging or reply payload. */
    unsigned char header[STYXRPC_NAMED_HEADER]; /* Partial request header. */
    size_t staged;                   /* Confirmed bytes in the current request. */
    size_t length;                   /* Request or reply body length. */
    size_t delivered;                /* Confirmed bytes in the current reply. */
    unsigned pending;                /* Read/write opcode while in the FIFO. */
    int ready;                       /* Reply available for transfer. */
    int failed;                      /* Terminal protocol error on this endpoint. */
};


/* EN/RU: See named_channel.h. / См. named_channel.h. */
StyxRpcNamedRegistry *styxrpc_named_registry(StyxRpcNamedClaim claim,
    StyxRpcNamedVisible visible, StyxRpcNamedDrop drop, void *state)
{
    if ((claim != NULL) != (drop != NULL))
    {
        return NULL;
    }
    StyxRpcNamedRegistry *registry = calloc(1, sizeof(*registry));
    if (registry && pthread_mutex_init(&registry->lock, NULL))
    {
        free(registry);
        registry = NULL;
    }
    if (registry && pthread_mutex_init(&registry->names_lock, NULL))
    {
        pthread_mutex_destroy(&registry->lock);
        free(registry);
        registry = NULL;
    }
    if (registry)
    {
        registry->claim = claim;
        registry->visible = visible;
        registry->drop = drop;
        registry->state = state;
    }
    return registry;
}


/* EN/RU: See named_channel.h. / См. named_channel.h. */
void styxrpc_named_destroy(StyxRpcNamedRegistry *registry)
{
    if (registry)
    {
        pthread_mutex_destroy(&registry->names_lock);
        pthread_mutex_destroy(&registry->lock);
        free(registry);
    }
}


/* EN/RU: See named_channel.h. / См. named_channel.h. */
StyxRpcNamedEndpoint *styxrpc_named_endpoint(StyxRpcNamedRegistry *registry)
{
    StyxRpcNamedEndpoint *endpoint = registry ? calloc(1, sizeof(*endpoint)) : NULL;
    if (endpoint)
    {
        endpoint->registry = registry;
    }
    return endpoint;
}


/* EN: Remove endpoint from its channel's queue under the registry lock.
 * RU: Удалить endpoint из очереди канала под блокировкой реестра. */
static void unlink_waiter(NamedChannel *channel, StyxRpcNamedEndpoint *endpoint)
{
    StyxRpcNamedEndpoint *previous = NULL, *current = channel->head;
    while (current && current != endpoint)
    {
        previous = current;
        current = current->next;
    }
    if (current)
    {
        if (previous)
        {
            previous->next = current->next;
        }
        else
        {
            channel->head = current->next;
        }
        if (channel->tail == current)
        {
            channel->tail = previous;
        }
    }
}


/* EN/RU: See named_channel.h. / См. named_channel.h. */
void styxrpc_named_retire(StyxRpcNamedEndpoint *endpoint)
{
    if (!endpoint)
    {
        return;
    }
    StyxRpcNamedRegistry *registry = endpoint->registry;
    NamedChannel *channel = endpoint->channel;
    if (channel)
    {
        pthread_mutex_lock(&registry->lock);
        if (endpoint->pending)
        {
            unlink_waiter(channel, endpoint);
        }
        pthread_mutex_unlock(&registry->lock);
        pthread_mutex_lock(&registry->names_lock);
        if (!--channel->participants)
        {
            NamedChannel **link = &registry->names;
            while (*link != channel)
            {
                link = &(*link)->next;
            }
            *link = channel->next;
            if (registry->drop)
            {
                registry->drop(registry->state, channel->path, channel->lease);
            }
            free(channel);
        }
        pthread_mutex_unlock(&registry->names_lock);
    }
    free(endpoint->body);
    free(endpoint);
}


/* EN: Join or create the name once VFS admission has succeeded.
 * RU: Присоединить участника или создать имя после проверки VFS. */
static int enroll(StyxRpcNamedEndpoint *endpoint)
{
    char path[STYXRPC_VFS_PATH_MAX];
    if (endpoint->channel || !endpoint->length || endpoint->length >= sizeof(path) ||
        memchr(endpoint->body, 0, endpoint->length))
    {
        return 0;
    }
    memcpy(path, endpoint->body, endpoint->length);
    path[endpoint->length] = 0;
    StyxRpcNamedRegistry *registry = endpoint->registry;
    if (!styxrpc_named_path(path))
    {
        return 0;
    }
    pthread_mutex_lock(&registry->names_lock);
    NamedChannel *channel = registry->names;
    while (channel && strcmp(channel->path, path))
    {
        channel = channel->next;
    }
    if (channel && registry->visible && !registry->visible(registry->state, path, channel->lease))
    {
        pthread_mutex_unlock(&registry->names_lock);
        return 0;
    }
    if (!channel)
    {
        channel = calloc(1, sizeof(*channel));
        if (channel)
        {
            channel->lease = registry->claim ? registry->claim(registry->state, path) : 1;
            if (channel->lease)
            {
                strcpy(channel->path, path);
                channel->next = registry->names;
                registry->names = channel;
            }
            else
            {
                free(channel);
                channel = NULL;
            }
        }
    }
    if (channel)
    {
        ++channel->participants;
        endpoint->channel = channel;
    }
    pthread_mutex_unlock(&registry->names_lock);
    free(endpoint->body);
    endpoint->body = NULL;
    endpoint->length = 0;
    return channel != NULL;
}


/* EN: Move the staged frame to the oldest opposite operation, or enqueue it.
 * RU: Передать подготовленный кадр старейшей встречной операции либо поставить в очередь. */
static void pair_or_wait(StyxRpcNamedEndpoint *endpoint, unsigned operation)
{
    StyxRpcNamedRegistry *registry = endpoint->registry;
    pthread_mutex_lock(&registry->lock);
    NamedChannel *channel = endpoint->channel;
    StyxRpcNamedEndpoint *peer = channel->head;
    if (peer && peer->pending != operation)
    {
        StyxRpcNamedEndpoint *writer = operation == STYXRPC_NAMED_WRITE ? endpoint : peer;
        StyxRpcNamedEndpoint *reader = writer == endpoint ? peer : endpoint;
        unlink_waiter(channel, peer);
        reader->body = writer->body;
        reader->length = writer->length;
        writer->body = NULL;
        writer->length = 0;
        peer->pending = 0;
        writer->ready = 1;
        reader->ready = 1;
    }
    else
    {
        endpoint->pending = operation;
        endpoint->next = NULL;
        if (channel->tail)
        {
            channel->tail->next = endpoint;
        }
        else
        {
            channel->head = endpoint;
        }
        channel->tail = endpoint;
    }
    pthread_mutex_unlock(&registry->lock);
}


/* EN: Decode a complete request header and allocate the exact sized staging buffer.
 * RU: Разобрать полный заголовок и выделить буфер точной длины. */
static int begin_body(StyxRpcNamedEndpoint *endpoint)
{
    const unsigned char *header = endpoint->header;
    unsigned operation = header[0];
    size_t length = ((uint32_t)header[1] << 24) | ((uint32_t)header[2] << 16) |
        ((uint32_t)header[3] << 8) | header[4];
    if (endpoint->channel ?
        !((operation == STYXRPC_NAMED_READ && !length) ||
          (operation == STYXRPC_NAMED_WRITE && length && length <= STYXRPC_NAMED_MAX)) :
        !(operation == STYXRPC_NAMED_OPEN && length && length < STYXRPC_VFS_PATH_MAX))
    {
        return 0;
    }
    endpoint->length = length;
    endpoint->body = length ? malloc(length) : NULL;
    return !length || endpoint->body != NULL;
}


/* EN/RU: See named_channel.h. / См. named_channel.h. */
int styxrpc_named_submit(StyxRpcNamedEndpoint *endpoint, uint64_t offset, const void *data, size_t size)
{
    /* Endpoint I/O is serialized by its fid. Only pairing can touch a pending endpoint. */
    pthread_mutex_lock(&endpoint->registry->lock);
    int idle = !endpoint->pending && !endpoint->ready && !endpoint->failed;
    pthread_mutex_unlock(&endpoint->registry->lock);
    if (!idle || !data || !size || offset != endpoint->staged)
    {
        return 0;
    }
    const unsigned char *bytes = data;
    size_t prefix = endpoint->staged < STYXRPC_NAMED_HEADER ? STYXRPC_NAMED_HEADER - endpoint->staged : 0;
    if (prefix)
    {
        size_t copied = prefix < size ? prefix : size;
        memcpy(endpoint->header + endpoint->staged, bytes, copied);
        endpoint->staged += copied;
        size -= copied;
        bytes += copied;
        if (endpoint->staged < STYXRPC_NAMED_HEADER)
        {
            return 1;
        }
        if (!begin_body(endpoint))
        {
            endpoint->failed = 1;
            return 0;
        }
    }
    size_t filled = endpoint->staged - STYXRPC_NAMED_HEADER;
    if (size > endpoint->length - filled)
    {
        endpoint->failed = 1;
        return 0;
    }
    if (size)
    {
        memcpy(endpoint->body + filled, bytes, size);
        endpoint->staged += size;
    }
    if (filled + size == endpoint->length)
    {
        endpoint->staged = 0;
        if (endpoint->header[0] == STYXRPC_NAMED_OPEN)
        {
            endpoint->failed = !enroll(endpoint);
            return !endpoint->failed;
        }
        pair_or_wait(endpoint, endpoint->header[0]);
    }
    return 1;
}


/* EN/RU: See named_channel.h. / См. named_channel.h. */
int styxrpc_named_reply(StyxRpcNamedEndpoint *endpoint, uint64_t offset, void *data, size_t capacity, size_t *used)
{
    StyxRpcNamedRegistry *registry = endpoint->registry;
    unsigned char *bytes = data;
    pthread_mutex_lock(&registry->lock);
    int valid = !endpoint->failed && offset == endpoint->delivered && (endpoint->pending || endpoint->ready);
    *used = 0;
    if (valid && endpoint->ready)
    {
        size_t end = STYXRPC_NAMED_REPLY + endpoint->length;
        size_t next = endpoint->delivered;
        while (next < STYXRPC_NAMED_REPLY && capacity)
        {
            *bytes++ = (unsigned char)(endpoint->length >> (8U * (STYXRPC_NAMED_REPLY - next - 1)));
            ++next;
            --capacity;
        }
        size_t remaining = end - next;
        size_t chunk = remaining < capacity ? remaining : capacity;
        if (chunk)
        {
            memcpy(bytes, endpoint->body + next - STYXRPC_NAMED_REPLY, chunk);
            next += chunk;
        }
        *used = next - endpoint->delivered;
        endpoint->delivered = next;
        if (next == end)
        {
            free(endpoint->body);
            endpoint->body = NULL;
            endpoint->ready = 0;
            endpoint->delivered = 0;
            endpoint->length = 0;
        }
    }
    pthread_mutex_unlock(&registry->lock);
    return valid;
}
