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
#include <sys/socket.h>
#include <unistd.h>

#define NAMED_PEER_ROOT 0U /* Root Qid of the dedicated control session. */
#define NAMED_PEER_FILE 1U /* One fresh endpoint per open of the service file. */
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
};

static const StyxRpcEntry entries[] = {
    {NAMED_PEER_ROOT, "/", 0555, 1},
    {NAMED_PEER_FILE, STYXRPC_NAMED_SERVICE, 0666, 0}
}; /* Private root and its single service file. */


/* EN: Return the sole immediate service entry under directory at index.
 * RU: Вернуть единственный служебный узел под directory по индексу index. */
static const StyxRpcEntry *entry(void *state, uint64_t directory, size_t index)
{
    (void)state;
    return directory == NAMED_PEER_ROOT && !index ? entries + NAMED_PEER_FILE : NULL;
}


/* EN: Describe id from immutable service metadata; copy it to node.
 * RU: Скопировать постоянные метаданные id в node. */
static int describe(void *state, uint64_t id, StyxRpcNode *node)
{
    (void)state;
    if (id > NAMED_PEER_FILE)
    {
        return 0;
    }
    *node = (StyxRpcNode){.entry = entries[id], .parent = NAMED_PEER_ROOT};
    return 1;
}


/* EN: Allocate the owned endpoint cookie for a read/write service fid.
 * RU: Выделить своё состояние участника для двустороннего управляющего fid. */
static int open_file(void *registry, uint64_t id, unsigned access, void **cookie)
{
    if (id != NAMED_PEER_FILE || access != (STYXRPC_READ | STYXRPC_WRITE))
    {
        return 0;
    }
    *cookie = styxrpc_named_endpoint(registry);
    return *cookie != NULL;
}


/* EN: Read a confirmed reply prefix at offset into bytes, bounded by capacity.
 * RU: Прочитать подтверждённый префикс ответа со смещения offset в границах capacity. */
static int read_file(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *used)
{
    return styxrpc_named_reply(cookie, offset, bytes, capacity, used);
}


/* EN: Stage count command bytes at offset; acknowledge only an accepted fragment.
 * RU: Принять count байтов команды со смещения offset и подтвердить принятый фрагмент. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *used)
{
    *used = 0;
    if (!styxrpc_named_submit(cookie, offset, bytes, count))
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
    styxrpc_named_retire(cookie);
}

static const StyxRpcTree tree = {{entry, open_file, read_file, write_file, close_file}, describe};
/* Both service instances borrow the same host registry. */


/* EN: Serve the private local end until closure or cancellation of cancel_fd.
 * RU: Обслуживать локальный конец до закрытия либо остановки через cancel_fd. */
static void *local_serve(void *state)
{
    StyxRpcNamedPeer *peer = state;
    StyxRpcServer *server = styxrpc_server_create(peer->local, &tree, peer->registry);
    while (styxrpc_server_step(server))
    {
    }
    __atomic_store_n(&peer->stopping, 1, __ATOMIC_RELEASE);
    styxrpc_server_destroy(server);
    return NULL;
}


/* EN: Serve the peer end and interrupt local waits when it disconnects.
 * RU: Обслуживать удалённый конец и прервать локальные ожидания при его отключении. */
static void *remote_serve(void *state)
{
    StyxRpcNamedPeer *peer = state;
    StyxRpcServer *server = styxrpc_server_create(peer->remote, &tree, peer->registry);
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
    StyxRpcNamedPeer *peer = lane && registry ? calloc(1, sizeof(*peer)) : NULL;
    if (!peer)
    {
        phyfree(lane);
        return NULL;
    }
    peer->registry = registry;
    peer->remote = lane;
    peer->cancel_fd = -1;
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
