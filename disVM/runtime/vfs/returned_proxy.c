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
#include <stdlib.h>
#include <string.h>


/* EN: Shared native lifetime of an exact returned capability group.
 * RU: Общее нативное время жизни группы возвращённых полномочий. */
struct DVM_ReturnedLease
{
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
    if (!lease->close || !lease->cancel)
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
    /* Decoder holds a pin and at most eight channel bindings. */
    __atomic_fetch_add(&lease->refs, 1, __ATOMIC_RELAXED);
    *proxy = (ReturnedProxy){.lease = lease, .schema = *schema};
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
void dvm_returned_proxy_dispose(void *binding)
{
    ReturnedProxy *proxy = binding;
    if (proxy)
    {
        dvm_returned_lease_release(proxy->lease);
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
