/* Copyright (c) 2026 bhgv
 * EN: Serialize a call's borrowed queues and revoke all its reverse fids at retirement.
 * RU: Сериализация заимствованных очередей вызова и отзыв встречных fid при завершении.
 */
#define _POSIX_C_SOURCE 200809L
#include "channel_arguments_internal.h"
#include "styxrpc/channel_route.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ARGUMENT_BUFFERS 2U /* One request and one response per opened fid. */

typedef struct ArgumentOpen ArgumentOpen; /* Bus operation handle retained until clunk. */

struct StyxRpcArgumentBus
{
    pthread_mutex_t lock;          /* Protects dispatcher operations and scope transitions. */
    StyxRpcArgumentScope *scope;  /* Exclusive staging or active invocation. */
    uint32_t pid;                   /* Fixed published reverse process. */
    uint32_t serial;                /* Last reserved key, never reused on this bus. */
};

struct StyxRpcArgumentScope
{
    StyxRpcArgumentBus *bus;          /* Borrowed until owner closes the scope. */
    StyxRpcChannelScheduler scheduler; /* Dispatcher operations under bus lock. */
    StyxRpcChannelQueue *owners[STYXRPC_CHANNEL_SLOTS]; /* Borrowed application queues. */
    StyxRpcChannelQueue queues[STYXRPC_CHANNEL_SLOTS]; /* Active copies sharing the byte arenas. */
    StyxRpcChannelGrant grants[STYXRPC_CHANNEL_SLOTS]; /* Immutable admitted argument keys. */
    ArgumentOpen *opens;              /* Intrusive list of this generation's operation fids. */
    size_t count;                     /* Distinct queues, in first occurrence order. */
    size_t direct[STYXRPC_CHANNEL_SLOTS]; /* Direct slots in first encoded order. */
    size_t direct_count;              /* Number of distinct direct argument queues. */
    int active;                       /* Zero until the complete call is ready. */
};

struct ArgumentOpen
{
    StyxRpcArgumentBus *bus;      /* Dispatcher and destruction remain serialized. */
    StyxRpcArgumentScope *scope;  /* Cleared on scope retirement before the scope is freed. */
    ArgumentOpen *next;           /* Next opened operation on this scope. */
    ArgumentOpen **previous;      /* Link referencing this operation. */
    StyxRpcChannelFid fid;         /* Checked request and immutable reply. */
    unsigned char bytes[];         /* Disjoint request and reply buffers. */
};


/* EN: Provide monotonic milliseconds for native WAIT selections.
 * RU: Предоставить монотонные миллисекунды для штатных веток WAIT. */
static uint64_t now_ms(void *unused)
{
    struct timespec now;
    (void)unused;
    return !clock_gettime(CLOCK_MONOTONIC, &now) ?
        UINT64_C(1000) * (uint64_t)now.tv_sec + (uint64_t)now.tv_nsec / 1000000 : 0;
}


/* EN: Cancel exactly one staged operation and detach it under the bus lock.
 * RU: Отменить одну подготовленную операцию и отсоединить её под блокировкой. */
static void detach(ArgumentOpen *opened)
{
    if (opened->scope)
    {
        styxrpc_channel_fid_close(&opened->fid);
        *opened->previous = opened->next;
        if (opened->next)
        {
            opened->next->previous = opened->previous;
        }
        opened->scope = NULL;
        opened->next = NULL;
        opened->previous = NULL;
    }
}


/* EN: Capture the active generation on an exact duplex operation fid.
 * RU: Захватить активное поколение при открытии fid чтения и записи. */
static int open_file(void *context, unsigned access, void **cookie)
{
    StyxRpcArgumentBus *bus = context;
    if (!bus || access != (STYXRPC_READ | STYXRPC_WRITE))
    {
        return 0;
    }
    ArgumentOpen *opened = calloc(1, sizeof(*opened) + ARGUMENT_BUFFERS * STYXRPC_MANAGED_LIMIT);
    if (!opened)
    {
        return 0;
    }
    pthread_mutex_lock(&bus->lock);
    StyxRpcArgumentScope *scope = bus->scope;
    int active = scope && scope->active && scope->count;
    if (active)
    {
        opened->bus = bus;
        opened->scope = scope;
        opened->fid = (StyxRpcChannelFid){
            .grants = scope->grants, .grant_count = scope->count,
            .request = opened->bytes, .request_capacity = STYXRPC_MANAGED_LIMIT,
            .reply = opened->bytes + STYXRPC_MANAGED_LIMIT, .reply_capacity = STYXRPC_MANAGED_LIMIT,
            .admit = styxrpc_scheduler_admit, .cancel = styxrpc_scheduler_cancel,
            .context = &scope->scheduler
        };
        opened->previous = &scope->opens;
        opened->next = scope->opens;
        if (opened->next)
        {
            opened->next->previous = &opened->next;
        }
        scope->opens = opened;
        *cookie = opened;
    }
    pthread_mutex_unlock(&bus->lock);
    if (!active)
    {
        free(opened);
    }
    return active;
}


/* EN: Accept only operations belonging to the generation captured at open.
 * RU: Принимать операции только поколения, захваченного при открытии. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    ArgumentOpen *opened = cookie;
    pthread_mutex_lock(&opened->bus->lock);
    int valid = opened->scope && offset <= SIZE_MAX &&
        styxrpc_channel_fid_write(&opened->fid, (size_t)offset, bytes, count) != STYXRPC_INVALID;
    pthread_mutex_unlock(&opened->bus->lock);
    if (valid)
    {
        *size = count;
    }
    return valid;
}


/* EN: Read a reverse response at the confirmed request extent, consistent with Dis.
 * RU: Читать встречный ответ после подтверждённого запроса, как в Dis. */
static int read_file(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    ArgumentOpen *opened = cookie;
    pthread_mutex_lock(&opened->bus->lock);
    int valid = 0;
    if (opened->scope && offset >= opened->fid.staged && offset - opened->fid.staged <= SIZE_MAX)
    {
        styxrpc_scheduler_progress(&opened->scope->scheduler);
        valid = styxrpc_channel_fid_read(&opened->fid, (size_t)(offset - opened->fid.staged),
            bytes, capacity, size) != STYXRPC_INVALID;
    }
    pthread_mutex_unlock(&opened->bus->lock);
    return valid;
}


/* EN: Close this exact operation even if its scope has already been revoked.
 * RU: Закрыть точную операцию, даже если её область уже отозвана. */
static void close_file(void *cookie)
{
    ArgumentOpen *opened = cookie;
    pthread_mutex_lock(&opened->bus->lock);
    detach(opened);
    pthread_mutex_unlock(&opened->bus->lock);
    free(opened);
}


/* EN/RU: See styxrpc/channel_arguments.h. / См. styxrpc/channel_arguments.h. */
StyxRpcArgumentBus *styxrpc_argument_bus_create(uint32_t pid)
{
    StyxRpcArgumentBus *bus = pid ? calloc(1, sizeof(*bus)) : NULL;
    if (bus && pthread_mutex_init(&bus->lock, NULL))
    {
        free(bus);
        return NULL;
    }
    if (bus)
    {
        bus->pid = pid;
    }
    return bus;
}


/* EN/RU: See styxrpc/channel_arguments.h. / См. styxrpc/channel_arguments.h. */
StyxRpcResource styxrpc_argument_bus_resource(StyxRpcArgumentBus *bus)
{
    return (StyxRpcResource){bus, open_file, read_file, write_file, close_file};
}


/* EN/RU: See styxrpc/channel_arguments.h. / См. styxrpc/channel_arguments.h. */
void styxrpc_argument_bus_destroy(StyxRpcArgumentBus *bus)
{
    if (bus)
    {
        styxrpc_argument_scope_close(bus->scope);
        pthread_mutex_destroy(&bus->lock);
        free(bus);
    }
}


/* EN/RU: See styxrpc/channel_arguments.h. / См. styxrpc/channel_arguments.h. */
StyxRpcArgumentScope *styxrpc_argument_scope_open(StyxRpcArgumentBus *bus, uint32_t pid)
{
    if (!bus || bus->pid != pid)
    {
        return NULL;
    }
    StyxRpcArgumentScope *scope = calloc(1, sizeof(*scope));
    if (!scope)
    {
        return NULL;
    }
    pthread_mutex_lock(&bus->lock);
    int available = !bus->scope;
    if (available)
    {
        scope->bus = bus;
        bus->scope = scope;
    }
    pthread_mutex_unlock(&bus->lock);
    if (!available)
    {
        free(scope);
        return NULL;
    }
    return scope;
}


/* EN: Reserve a fresh generation key for one exact queue and return its stable slot.
 * RU: Выделить свежий ключ поколения точной очереди и вернуть её стабильный индекс. */
static size_t reserve(StyxRpcArgumentScope *scope, StyxRpcChannelQueue *queue)
{
    if (!scope || !queue || scope->active)
    {
        return STYXRPC_CHANNEL_SLOTS;
    }
    size_t index = 0;
    while (index < scope->count && scope->owners[index] != queue)
    {
        ++index;
    }
    if (index == scope->count && index < STYXRPC_CHANNEL_SLOTS)
    {
        if (scope->bus->serial == INT32_MAX)
        {
            return STYXRPC_CHANNEL_SLOTS;
        }
        scope->grants[index] = queue->grant;
        scope->grants[index].key = ++scope->bus->serial;
        scope->owners[scope->count++] = queue;
    }
    return index;
}


/* EN/RU: See channel_arguments.h. / См. channel_arguments.h. */
StyxRpcStatus styxrpc_argument_scope_nested(StyxRpcArgumentScope *scope,
    StyxRpcChannelQueue *queue, StyxRpcChannelGrant *grant)
{
    size_t index = grant ? reserve(scope, queue) : STYXRPC_CHANNEL_SLOTS;
    if (index == STYXRPC_CHANNEL_SLOTS)
    {
        return STYXRPC_INVALID;
    }
    *grant = scope->grants[index];
    return STYXRPC_OK;
}


/* EN/RU: See channel_arguments.h. / См. channel_arguments.h. */
StyxRpcStatus styxrpc_argument_scope_value(StyxRpcArgumentScope *scope, StyxRpcChannelQueue *queue,
    void *bytes, size_t capacity, size_t *used)
{
    size_t before = scope ? scope->count : 0;
    size_t index = reserve(scope, queue);
    if (index == STYXRPC_CHANNEL_SLOTS)
    {
        return STYXRPC_INVALID;
    }
    const StyxRpcChannelRoute route = {.kind = STYXRPC_ROUTE_CURRENT, .channel = scope->grants[index]};
    StyxRpcStatus status = styxrpc_channel_route_value(&route, bytes, capacity, used);
    if (status == STYXRPC_OK)
    {
        size_t i = 0;
        while (i < scope->direct_count && scope->direct[i] != index)
        {
            ++i;
        }
        if (i == scope->direct_count)
        {
            scope->direct[scope->direct_count++] = index;
        }
    }
    else if (index == before)
    {
        --scope->count;
    }
    return status;
}


/* EN: Require each seeded leaf to be an exact ordinary key in this scope.
 * RU: Требовать точные обычные ключи области во всех начальных вложенных листьях. */
static int seed_valid(const StyxRpcArgumentScope *scope, const char *shape, const void *bytes, size_t size)
{
    StyxRpcChannelRoutes routes;
    if (styxrpc_channel_element_routes(shape, bytes, size, &routes) != STYXRPC_OK)
    {
        return 0;
    }
    for (size_t i = 0; i < routes.count; ++i)
    {
        const StyxRpcChannelRoute *route = routes.entries + i;
        size_t j = 0;
        while (j < scope->count && scope->grants[j].key != route->channel.key)
        {
            ++j;
        }
        if (route->kind != STYXRPC_ROUTE_CURRENT || route->unbuffered ||
            j == scope->count || strcmp(scope->grants[j].shape, route->channel.shape))
        {
            return 0;
        }
    }
    return 1;
}


/* EN/RU: See styxrpc/channel_arguments.h. / См. styxrpc/channel_arguments.h. */
int styxrpc_argument_scope_activate(StyxRpcArgumentScope *scope)
{
    if (!scope || scope->active)
    {
        return 0;
    }
    for (size_t i = 0; i < scope->count; ++i)
    {
        const StyxRpcChannelQueue *queue = scope->owners[i];
        if (queue->count > queue->capacity ||
            (queue->capacity ? queue->head >= queue->capacity : queue->head) ||
            !memchr(queue->grant.shape, 0, sizeof(queue->grant.shape)) ||
            strcmp(queue->grant.shape, scope->grants[i].shape))
        {
            return 0;
        }
        scope->queues[i] = *queue;
        scope->queues[i].grant = scope->grants[i];
        scope->queues[i].count = 0;
        scope->queues[i].head = 0;
    }
    if (scope->count && !styxrpc_scheduler_init(&scope->scheduler, scope->queues, scope->count, now_ms, NULL))
    {
        return 0;
    }
    for (size_t i = 0; i < scope->count; ++i)
    {
        const StyxRpcChannelQueue *queue = scope->owners[i];
        size_t slot = queue->head;
        for (size_t j = 0; j < queue->count; ++j)
        {
            if (!queue->sizes[slot] || queue->sizes[slot] > queue->stride ||
                (strchr(queue->grant.shape, 'c') && !seed_valid(scope, queue->grant.shape,
                    queue->bytes + slot * queue->stride, queue->sizes[slot])))
            {
                return 0;
            }
            slot = slot == queue->capacity - 1 ? 0 : slot + 1;
        }
        scope->queues[i].count = queue->count;
        scope->queues[i].head = queue->head;
    }
    pthread_mutex_lock(&scope->bus->lock);
    scope->active = 1;
    pthread_mutex_unlock(&scope->bus->lock);
    return 1;
}


/* EN/RU: See channel_arguments_internal.h. / См. channel_arguments_internal.h. */
int styxrpc_argument_scope_match(StyxRpcArgumentScope *scope, const StyxRpcChannelCall *call)
{
    if (!scope || scope->active || call->pid != scope->bus->pid || call->count != scope->direct_count)
    {
        return 0;
    }
    for (size_t i = 0; i < call->count; ++i)
    {
        const StyxRpcChannelGrant *grant = &call->grants[i];
        const StyxRpcChannelGrant *expected = scope->grants + scope->direct[i];
        if (grant->key != expected->key || strcmp(grant->shape, expected->shape))
        {
            return 0;
        }
    }
    return styxrpc_argument_scope_activate(scope);
}


/* EN/RU: See styxrpc/channel_arguments.h. / См. styxrpc/channel_arguments.h. */
void styxrpc_argument_scope_close(StyxRpcArgumentScope *scope)
{
    if (!scope)
    {
        return;
    }
    StyxRpcArgumentBus *bus = scope->bus;
    pthread_mutex_lock(&bus->lock);
    while (scope->opens)
    {
        detach(scope->opens);
    }
    if (scope->active)
    {
        for (size_t i = 0; i < scope->count; ++i)
        {
            scope->owners[i]->count = scope->queues[i].count;
            scope->owners[i]->head = scope->queues[i].head;
        }
    }
    bus->scope = NULL;
    pthread_mutex_unlock(&bus->lock);
    free(scope);
}
