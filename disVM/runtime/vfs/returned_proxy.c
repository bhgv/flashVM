/* Copyright (c) 2026 bhgv
 * EN: Hold a provider export alive until the last returned Dis proxy retires.
 * RU: Удержание выдачи провайдера до освобождения последнего прокси Dis.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "close_worker.h"
#include "returned_proxy.h"
#include "thread_port.h"
#include "channel_sync.h"
#include "disvm/refcount.h"
#include <stdlib.h>
#include <string.h>


/* EN: Shared native lifetime of an exact returned capability group.
 * RU: Общее нативное время жизни группы возвращённых полномочий. */
struct DVM_ReturnedLease
{
    FVM_PortSync *lock; /* Guards weak canonical channel entries. */
    struct ReturnedProxy *proxies; /* Live bindings, removed before freeing their channels. */
    unsigned refs; /* Atomic pins across channel owners and the decoder. */
    DVM_Provider *endpoint; /* Owned exact module UniBus file. */
    DVM_VfsHandle fid; /* Owned original function fid, closed off-thread. */
    DVM_CloseJob *close; /* Preallocated native retirement job. */
    DVM_Cancel *cancel; /* Scope identity and optional cancellation source. */
    unsigned char token[STYX_CHANNEL_EXPORT_TOKEN_SIZE]; /* Set by the first matched result leaf. */
    int token_set; /* Decoder-only flag before publishing the proxies. */
};


/* EN: One proxy owns a lease pin and its exact wire channel schema.
 * RU: Один прокси владеет ссылкой на аренду и точной схемой канала. */
typedef struct ReturnedProxy
{
    DVM_ReturnedLease *lease; /* Owned shared pin. */
    Channel *channel; /* Weak identity while this binding is registered. */
    struct ReturnedProxy *next; /* Next weak binding under the lease lock. */
    DVM_ChannelSchema schema; /* Immutable matched key and element type. */
} ReturnedProxy;


/* EN: Clunk the originating function and release its route on the native closer.
 * RU: Закрыть fid функции и освободить маршрут в нативном потоке закрытия. */
static void retire_lease(void *state)
{
    DVM_ReturnedLease *lease = state;
    dvm_vfs_close(&lease->fid);
    dvm_provider_release(lease->endpoint);
    dvm_cancel_release(lease->cancel);
    if (lease->lock)
    {
        fvm_port_sync_destroy(lease->lock);
    }
    free(lease);
}


/* EN/RU: See returned_proxy.h. / См. returned_proxy.h. */
DVM_ReturnedLease *dvm_returned_lease_create(DVM_Provider *endpoint)
{
    if (!endpoint || !dvm_provider_retain(endpoint))
    {
        return NULL;
    }
    DVM_ReturnedLease *lease = calloc(1, sizeof(*lease));
    if (!lease)
    {
        dvm_provider_release(endpoint);
        return NULL;
    }
    lease->endpoint = endpoint;
    lease->refs = 1;
    lease->close = dvm_close_prepare();
    lease->cancel = dvm_cancel_create();
    lease->lock = fvm_port_sync_create();
    if (!lease->close || !lease->cancel || !lease->lock)
    {
        dvm_close_discard(lease->close);
        retire_lease(lease);
        return NULL;
    }
    return lease;
}


/* EN/RU: See returned_proxy.h. / См. returned_proxy.h. */
int dvm_returned_lease_take(DVM_ReturnedLease *lease, DVM_VfsHandle *fid)
{
    return lease && dvm_vfs_move(&lease->fid, fid);
}


/* EN/RU: See returned_proxy.h. / См. returned_proxy.h. */
void dvm_returned_lease_release(DVM_ReturnedLease *lease)
{
    static const DVM_VfsOps cleanup = {.close = retire_lease}; /* Owned native lease cleanup. */
    if (lease && __atomic_sub_fetch(&lease->refs, 1, __ATOMIC_ACQ_REL) == 0)
    {
        DVM_VfsHandle resource = {.ops = &cleanup, .state = lease};
        dvm_close_submit_resource(lease->close, &resource);
    }
}


/* EN/RU: See returned_proxy.h. / См. returned_proxy.h. */
int dvm_returned_lease_token(DVM_ReturnedLease *lease, const unsigned char *token)
{
    if (!lease || !lease->fid.ops)
    {
        return 0;
    }
    if (!lease->token_set)
    {
        memcpy(lease->token, token, sizeof(lease->token));
        lease->token_set = 1;
    }
    return !memcmp(lease->token, token, sizeof(lease->token));
}


/* EN/RU: See returned_proxy.h. / См. returned_proxy.h. */
int dvm_returned_proxy_arm(Channel *channel, DVM_ReturnedLease *lease, const DVM_ChannelSchema *schema)
{
    ReturnedProxy *proxy = malloc(sizeof(*proxy));
    if (!proxy)
    {
        return 0;
    }
    /* The decoder or receiving guard already holds a lease pin. */
    __atomic_fetch_add(&lease->refs, 1, __ATOMIC_RELAXED);
    *proxy = (ReturnedProxy){.lease = lease, .channel = channel, .schema = *schema};
    channel->aux = proxy;
    return 1;
}


/* EN/RU: See returned_proxy.h. / См. returned_proxy.h. */
const DVM_ChannelSchema *dvm_returned_proxy_route(const Channel *channel, DVM_Provider **endpoint,
    DVM_Cancel **cancel, const unsigned char **token)
{
    const ReturnedProxy *proxy = channel->aux;
    if (!proxy)
    {
        return NULL;
    }
    *endpoint = proxy->lease->endpoint;
    *cancel = proxy->lease->cancel;
    *token = proxy->lease->token;
    return &proxy->schema;
}


/* EN/RU: See returned_proxy.h. / См. returned_proxy.h. */
DVM_ReturnedLease *dvm_returned_proxy_lease(const Channel *channel)
{
    const ReturnedProxy *proxy = channel->aux;
    return proxy ? proxy->lease : NULL;
}


/* EN/RU: See returned_proxy.h. / См. returned_proxy.h. */
Channel *dvm_returned_proxy_canonical(Channel *candidate)
{
    ReturnedProxy *binding = candidate->aux;
    DVM_ReturnedLease *lease = binding->lease;
    Channel *result = candidate;
    fvm_port_sync_lock(lease->lock);
    for (ReturnedProxy *entry = lease->proxies; entry; entry = entry->next)
    {
        if (entry->schema.key == binding->schema.key)
        {
            if (strcmp(entry->schema.shape, binding->schema.shape))
            {
                result = NULL;
                break;
            }
            if (dvm_refcount_retain_u(&D2H(entry->channel)->ref) == DVM_REFCOUNT_LIVE)
            {
                result = entry->channel;
                Setmark(D2H(result));
                break;
            }
        }
    }
    if (result == candidate)
    {
        binding->next = lease->proxies;
        lease->proxies = binding;
    }
    fvm_port_sync_unlock(lease->lock);
    return result;
}


/* EN/RU: See returned_proxy.h. / См. returned_proxy.h. */
void dvm_returned_proxy_dispose(void *binding)
{
    ReturnedProxy *proxy = binding;
    if (proxy)
    {
        DVM_ReturnedLease *lease = proxy->lease;
        fvm_port_sync_lock(lease->lock);
        ReturnedProxy **entry = &lease->proxies;
        while (*entry && *entry != proxy)
        {
            entry = &(*entry)->next;
        }
        if (*entry)
        {
            *entry = proxy->next;
        }
        fvm_port_sync_unlock(lease->lock);
        dvm_returned_lease_release(lease);
        free(proxy);
    }
}


/* [GC:] EN/RU: See returned_proxy.h. / См. returned_proxy.h. */
void dvm_returned_import_clear(DVM_ReturnedImport *imports)
{
    while (imports->count)
    {
        Channel *object = imports->objects[--imports->count];
        ptrdel(D2H(object));
        destroy(object);
    }
    dvm_returned_lease_release(imports->lease);
    imports->lease = NULL;
}
