/* Copyright (c) 2026 bhgv
 * EN: Run two native Styx dispatchers for one registry with no guest callbacks.
 * RU: Два нативных диспетчера Styx одного реестра без колбэков гостя.
 */
#define _POSIX_C_SOURCE 200809L /* F_DUPFD_CLOEXEC on the hosted POSIX target. */
#include "styxrpc/named_peer.h"
#include "styxrpc/serve.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define NAMED_PEER_ROOT 0U /* Root Qid of the dedicated control session. */
#define NAMED_PEER_FILE 1U /* One fresh endpoint per open of the service file. */
#define NAMED_PEER_SCOPE 2U /* Optional read-only nonce of the named connection. */
#define NAMED_PEER_WAIT_MS 50 /* Bound the remote worker's shutdown check. */
#define NAMED_PEER_STREAM (PHY_STREAM | PHY_ORDERED | PHY_RELIABLE) /* Ordered POSIX socket pair. */

struct StyxRpcNamedPeer
{
    StyxRpcNamedRegistry *registry; /* Borrowed until both dispatchers finish. */
    StyxClient *client;            /* Owned local client mount. */
    RemotePhy *remote;             /* Given to the remote worker after start. */
    RemotePhy *local;              /* Given to the local worker after start. */
    int cancel_fd;                 /* Owned duplicate for interrupting the local stream. */
    pthread_t remote_worker;       /* Owns and retires the delegated server. */
    pthread_t local_worker;        /* Owns and retires the local server. */
    int remote_started;            /* Owner must join the remote worker. */
    int local_started;             /* Owner must join the local worker. */
    int stopping;                  /* Atomic shutdown request to the remote worker. */
    int scoped;                    /* Immutable after service owners start. */
    int ref_client;                 /* Atomic flag for the remote named attachment. */
    unsigned char scope[STYXRPC_MODULE_REF_SCOPE_SIZE]; /* Immutable paired scope. */
};

typedef struct NamedPeerFile
{
    StyxRpcNamedEndpoint *endpoint; /* Owned channel or cell command endpoint. */
    const unsigned char *scope;     /* Borrowed bytes when opened on the scope node. */
} NamedPeerFile;

static const StyxRpcEntry entries[] = {
    {NAMED_PEER_ROOT, "/", 0555, 1},
    {NAMED_PEER_FILE, STYXRPC_NAMED_SERVICE, 0666, 0},
    {NAMED_PEER_SCOPE, STYXRPC_MODULE_REF_SCOPE_FILE, 0444, 0}
}; /* Private root, service file and optional scope. */


/* EN: Return an immediate service entry under directory at index.
 * RU: Вернуть служебный узел под directory по индексу index. */
static const StyxRpcEntry *entry(void *state, uint64_t directory, size_t index)
{
    const StyxRpcNamedPeer *peer = state;
    return directory == NAMED_PEER_ROOT && index < 1U + peer->scoped ? entries + index + 1 : NULL;
}


/* EN: Describe id from immutable service metadata; copy it to node.
 * RU: Скопировать постоянные метаданные id в node. */
static int describe(void *state, uint64_t id, StyxRpcNode *node)
{
    const StyxRpcNamedPeer *peer = state;
    if (id > NAMED_PEER_SCOPE || (id == NAMED_PEER_SCOPE && !peer->scoped))
    {
        return 0;
    }
    *node = (StyxRpcNode){.entry = entries[id], .parent = NAMED_PEER_ROOT,
        .length = id == NAMED_PEER_SCOPE ? STYXRPC_MODULE_REF_SCOPE_SIZE : 0};
    return 1;
}


/* EN: Allocate a private service or scope cookie, enforcing its exact access.
 * RU: Выделить состояние службы или области и проверить его точный доступ. */
static int open_file(void *state, uint64_t id, unsigned access, void **cookie)
{
    StyxRpcNamedPeer *peer = state;
    int endpoint = id == NAMED_PEER_FILE && access == (STYXRPC_READ | STYXRPC_WRITE);
    int scope = id == NAMED_PEER_SCOPE && peer->scoped && access == STYXRPC_READ;
    NamedPeerFile *file = endpoint || scope ? calloc(1, sizeof(*file)) : NULL;
    if (!file)
    {
        return 0;
    }
    if (endpoint && !(file->endpoint = styxrpc_named_endpoint(peer->registry)))
    {
        free(file);
        return 0;
    }
    file->scope = scope ? peer->scope : NULL;
    *cookie = file;
    return 1;
}


/* EN: Read a confirmed reply prefix at offset into bytes, bounded by capacity.
 * RU: Прочитать подтверждённый префикс ответа со смещения offset в границах capacity. */
static int read_file(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *used)
{
    NamedPeerFile *file = cookie;
    if (file->endpoint)
    {
        return styxrpc_named_reply(file->endpoint, offset, bytes, capacity, used);
    }
    size_t remaining = offset < STYXRPC_MODULE_REF_SCOPE_SIZE ? STYXRPC_MODULE_REF_SCOPE_SIZE - (size_t)offset : 0;
    *used = remaining < capacity ? remaining : capacity;
    if (*used)
    {
        memcpy(bytes, file->scope + (size_t)offset, *used);
    }
    return 1;
}


/* EN: Stage count command bytes at offset; acknowledge only an accepted fragment.
 * RU: Принять count байтов команды со смещения offset и подтвердить принятый фрагмент. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *used)
{
    *used = 0;
    NamedPeerFile *file = cookie;
    if (!file->endpoint || !styxrpc_named_submit(file->endpoint, offset, bytes, count))
    {
        return 0;
    }
    *used = count;
    return 1;
}


/* EN: Consume the endpoint owned by this exact fid.
 * RU: Освободить участника, которым владеет этот fid. */
static void close_file(void *cookie)
{
    NamedPeerFile *file = cookie;
    styxrpc_named_retire(file->endpoint);
    free(file);
}

static const StyxRpcTree tree = {{entry, open_file, read_file, write_file, close_file}, describe};
/* Both service instances borrow the same host registry. */


/* EN: Serve the private local end until closure or cancellation of cancel_fd.
 * RU: Обслуживать локальный конец до закрытия либо остановки через cancel_fd. */
static void *local_serve(void *state)
{
    StyxRpcNamedPeer *peer = state;
    StyxRpcServer *server = styxrpc_server_create(peer->local, &tree, peer);
    while (styxrpc_server_step(server))
    {
    }
    __atomic_store_n(&peer->stopping, 1, __ATOMIC_RELEASE);
    styxrpc_server_destroy(server);
    return NULL;
}


/* EN: Record module-ref support on the remote server's owner before Rattach.
 * RU: Отметить поддержку ссылок до Rattach у владельца удалённого сервера. */
static void observe_ref_client(void *state, const char *name)
{
    StyxRpcNamedPeer *peer = state;
    __atomic_store_n(&peer->ref_client, !strcmp(name, STYXRPC_MODULE_REF_ATTACH), __ATOMIC_RELEASE);
}


/* EN: Serve the peer end and interrupt local waits when it disconnects.
 * RU: Обслуживать удалённый конец и прервать локальные ожидания при его отключении. */
static void *remote_serve(void *state)
{
    StyxRpcNamedPeer *peer = state;
    StyxRpcServer *server = styxrpc_server_create(peer->remote, &tree, peer);
    if (peer->scoped)
    {
        styxrpc_server_observe_attach(server, observe_ref_client, peer);
    }
    while (!__atomic_load_n(&peer->stopping, __ATOMIC_ACQUIRE) &&
        styxrpc_server_step_for(server, NAMED_PEER_WAIT_MS))
    {
    }
    shutdown(peer->cancel_fd, SHUT_RDWR);
    styxrpc_server_destroy(server);
    return NULL;
}


/* EN/RU: See named_peer.h. / См. named_peer.h. */
void styxrpc_named_peer_destroy(StyxRpcNamedPeer *peer)
{
    if (!peer)
    {
        return;
    }
    __atomic_store_n(&peer->stopping, 1, __ATOMIC_RELEASE);
    if (peer->cancel_fd >= 0)
    {
        shutdown(peer->cancel_fd, SHUT_RDWR);
    }
    styxclientshutdown(peer->client);
    styxclientfree(peer->client);
    if (peer->local_started)
    {
        pthread_join(peer->local_worker, NULL);
    }
    else
    {
        phyfree(peer->local);
    }
    if (peer->remote_started)
    {
        pthread_join(peer->remote_worker, NULL);
    }
    else
    {
        phyfree(peer->remote);
    }
    if (peer->cancel_fd >= 0)
    {
        close(peer->cancel_fd);
    }
    free(peer);
}


/* EN/RU: See named_peer.h. / См. named_peer.h. */
StyxRpcNamedPeer *styxrpc_named_peer_host(RemotePhy *lane, StyxRpcNamedRegistry *registry,
    unsigned msize, unsigned timeout, unsigned tags)
{
    return styxrpc_named_peer_host_scoped(lane, registry, msize, timeout, tags, NULL);
}


/* EN/RU: See named_peer.h. / См. named_peer.h. */
StyxRpcNamedPeer *styxrpc_named_peer_host_scoped(RemotePhy *lane, StyxRpcNamedRegistry *registry,
    unsigned msize, unsigned timeout, unsigned tags, const void *scope)
{
    StyxRpcNamedPeer *peer = lane && registry ? calloc(1, sizeof(*peer)) : NULL;
    if (!peer)
    {
        phyfree(lane);
        return NULL;
    }
    peer->registry = registry;
    peer->remote = lane;
    peer->cancel_fd = -1;
    if (scope)
    {
        peer->scoped = 1;
        memcpy(peer->scope, scope, sizeof(peer->scope));
    }
    int local[2];
    if (!socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, local))
    {
        peer->cancel_fd = fcntl(local[0], F_DUPFD_CLOEXEC, 0);
        peer->local = phyposixfd(local[0], NAMED_PEER_STREAM);
        RemotePhy *caller = phyposixfd(local[1], NAMED_PEER_STREAM);
        if (peer->cancel_fd >= 0 && peer->local && caller &&
            !pthread_create(&peer->remote_worker, NULL, remote_serve, peer))
        {
            peer->remote_started = 1;
            if (!pthread_create(&peer->local_worker, NULL, local_serve, peer))
            {
                peer->local_started = 1;
                peer->client = styxclientmountphy(caller, msize, "rpc", "", timeout, tags);
                caller = NULL;
            }
        }
        phyfree(caller);
    }
    if (!peer->client)
    {
        styxrpc_named_peer_destroy(peer);
        return NULL;
    }
    return peer;
}


/* EN/RU: See named_peer.h. / См. named_peer.h. */
StyxClient *styxrpc_named_peer_client(StyxRpcNamedPeer *peer)
{
    return peer ? peer->client : NULL;
}


/* EN/RU: See named_peer.h. / См. named_peer.h. */
int styxrpc_named_peer_ref_scope(StyxRpcNamedPeer *peer, void *scope)
{
    if (!peer || !scope || !peer->scoped || !__atomic_load_n(&peer->ref_client, __ATOMIC_ACQUIRE))
    {
        return 0;
    }
    memcpy(scope, peer->scope, sizeof(peer->scope));
    return 1;
}


/* EN/RU: See named_peer.h. / См. named_peer.h. */
int styxrpc_named_peer_read_ref_scope(StyxClient *client, void *scope)
{
    if (!client || !scope)
    {
        return 0;
    }
    StyxFid *file = styxclientwalk(client, STYXRPC_MODULE_REF_SCOPE_FILE);
    StyxMeta meta;
    int valid = file && styxclientstat(file, &meta) && !meta.directory &&
        meta.length == STYXRPC_MODULE_REF_SCOPE_SIZE && styxclientopen(file, STYXRPC_FILE_READ);
    if (valid)
    {
        unsigned char bytes[STYXRPC_MODULE_REF_SCOPE_SIZE];
        StyxTransfer transfer;
        styxclientread(file, 0, bytes, sizeof(bytes), &transfer);
        valid = transfer.success && transfer.confirmed && !transfer.uncertain && transfer.count == sizeof(bytes);
        if (valid)
        {
            memcpy(scope, bytes, sizeof(bytes));
        }
    }
    styxclientclunk(file);
    return valid;
}
