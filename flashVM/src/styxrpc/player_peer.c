/* Copyright (c) 2026 bhgv
 * EN: Mount a peer without blocking the opposite Styx server's owner.
 * RU: Монтирование peer без блокировки владельца встречного сервера Styx.
 */
#include "player_peer.h"
#include "../../ports/monotonic_port.h"
#include "../../ports/network_port.h"
#include "../../ports/thread_port.h"
#include "styxrpc/named_cell.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLAYER_PEER_POLL_MS 20 /* Maximum owner wait between mount checks. */
#define PLAYER_FIRST_WORKER_PID 1U  /* First logical caller ID in this connection's reverse tree. */


/* EN: Install the exact PID and native argument bus in the shared program registry.
 * RU: Установить точный PID и нативную шину аргументов в общий реестр программ. */
static int add_program(PlayerPrograms *programs, FVM_StyxRpcWorkerScope *scope, uint32_t pid)
{
    StyxRpcResource resource;
    scope->pid = pid;
    scope->bus = styxrpc_argument_bus_create(pid);
    if (!scope->bus)
    {
        return 0;
    }
    resource = styxrpc_argument_bus_resource(scope->bus);
    return styxrpc_program_add(&programs->registry, pid) &&
        styxrpc_program_unibus(&programs->registry, pid, &resource);
}


/* EN/RU: See player_peer.h. / См. player_peer.h. */
int fvm_styxrpc_player_programs(FVM_StyxRpcPlayback *playback, PlayerPrograms *programs, int dis_peer)
{
    size_t i;
    if (!styxrpc_programs_init(&programs->registry, programs->nodes, PLAYER_PROGRAM_SLOTS))
    {
        return 0;
    }
    for (i = 0; i < FVM_ASYNC_MAX_TASKS; ++i)
    {
        if (!add_program(programs, &programs->scopes.entries[i], PLAYER_FIRST_WORKER_PID + (uint32_t)i))
        {
            return 0;
        }
    }
    if ((dis_peer && !add_program(programs, &programs->movie, PLAYER_FIRST_WORKER_PID + FVM_ASYNC_MAX_TASKS)) ||
        !styxrpc_module_slot_programs(playback->slot, &programs->registry))
    {
        return 0;
    }
    playback->movie_scope = dis_peer ? &programs->movie : NULL;
    playback->worker_scopes = &programs->scopes;
    return 1;
}


/* EN/RU: See player_peer.h. / См. player_peer.h. */
void fvm_styxrpc_player_programs_close(PlayerPrograms *programs)
{
    size_t i;
    styxrpc_argument_bus_destroy(programs->movie.bus);
    for (i = 0; i < FVM_ASYNC_MAX_TASKS; ++i)
    {
        styxrpc_argument_bus_destroy(programs->scopes.entries[i].bus);
    }
}


/* EN: The mount worker owns lane. The owner reads result after joining.
 * RU: Поток монтирования владеет каналом. Владелец читает результат после join. */
typedef struct PeerMount
{
    RemotePhy *lane;      /* Consumed by the blocking Styx mount. */
    pthread_mutex_t lock; /* Protects the completion notification. */
    int done;             /* Mount has completed, including possible failure. */
    StyxClient *result;   /* Owned client, transferred after worker termination. */
    int attached;         /* Owner observed the first opposite attach. */
    int named;            /* Owner observed the named service capability. */
} PeerMount;


/* EN: Negotiate the client lane, then publish completion without entering AS.
 * RU: Подключить клиентский канал и опубликовать завершение без входа в AS. */
static void mount_lane(void *state)
{
    PeerMount *mount = state;
    StyxClient *result = styxclientmountphy(mount->lane, PLAYER_MSIZE, "flash",
        STYXRPC_NAMED_PEER_ATTACH, PLAYER_WAIT_MS, PLAYER_TAGS);
    pthread_mutex_lock(&mount->lock);
    mount->result = result;
    mount->done = 1;
    pthread_mutex_unlock(&mount->lock);
}


/* EN: Record capability on the first incoming attach without entering a movie.
 * RU: Записать возможность первого входящего attach без входа в фильм. */
static void peer_attach(void *state, const char *name)
{
    PeerMount *mount = state;
    if (!mount->attached)
    {
        mount->attached = 1;
        mount->named = !strcmp(name, STYXRPC_NAMED_PEER_ATTACH);
    }
}


/* EN/RU: See player_peer.h. / См. player_peer.h. */
StyxClient *fvm_styxrpc_player_peer_mount(StyxRpcServer *server, RemotePhy *lane, int *named)
{
    PeerMount mount = {0};
    FVM_PortThread *worker;
    int serving = 1;
    double deadline = 0;
    *named = 0;
    if (!lane || pthread_mutex_init(&mount.lock, NULL))
    {
        phyfree(lane);
        return NULL;
    }
    mount.lane = lane;
    styxrpc_server_observe_attach(server, peer_attach, &mount);
    worker = fvm_port_thread_start(mount_lane, &mount);
    if (!worker)
    {
        phyfree(lane);
    }
    else
    {
        while (serving)
        {
            int done;
            int failed;
            pthread_mutex_lock(&mount.lock);
            done = mount.done;
            failed = done && !mount.result;
            pthread_mutex_unlock(&mount.lock);
            if (failed)
            {
                break;
            }
            if (done)
            {
                StyxRpcServerState state;
                double now;
                if (mount.attached && styxrpc_server_state(server, &state) && !state.reply_bytes)
                {
                    *named = mount.named;
                    break;
                }
                if (!fvm_port_monotonic_ms(&now) || (deadline && now >= deadline))
                {
                    break;
                }
                if (!deadline)
                {
                    deadline = now + PLAYER_WAIT_MS;
                }
            }
            serving = styxrpc_server_step_for(server, PLAYER_PEER_POLL_MS);
        }
        fvm_port_thread_join(worker);
    }
    styxrpc_server_observe_attach(server, NULL, NULL);
    pthread_mutex_destroy(&mount.lock);
    if (!serving)
    {
        styxclientshutdown(mount.result);
        styxclientfree(mount.result);
        return NULL;
    }
    return mount.result;
}


/* EN: Fill a capability token with OS entropy, without entering either VM.
 * RU: Заполнить жетон случайными байтами ОС без входа в любую VM. */
static int cell_entropy(void *state, unsigned char *bytes, size_t size)
{
    (void)state;
    return fvm_port_secure_random(bytes, size);
}


/* EN/RU: See player_peer.h. / См. player_peer.h. */
int fvm_styxrpc_player_peer_channels(FVM_StyxRpcPlayback *playback, PhyBidir *mux,
    unsigned serve_lane, PlayerPeerChannels *channels)
{
    RemotePhy *lane = phybidirchannel(mux, STYXRPC_NAMED_PEER_LANE);
    if (!lane)
    {
        return 0;
    }
    channels->refs.slot = playback->slot;
    channels->refs.lane = serve_lane == PLAYER_SERVE_FORWARD ?
        STYXRPC_MODULE_REF_FORWARD : STYXRPC_MODULE_REF_REVERSE;
    if (serve_lane == PLAYER_SERVE_FORWARD)
    {
        unsigned char scope[STYXRPC_MODULE_REF_SCOPE_SIZE];
        const void *offer;
        channels->registry = styxrpc_module_slot_named_registry(playback->slot);
        if (!styxrpc_named_cells_enable(channels->registry, cell_entropy, NULL))
        {
            phyfree(lane);
            return 0;
        }
        offer = fvm_port_secure_random(scope, sizeof(scope)) ? scope : NULL;
        channels->host = styxrpc_named_peer_host_scoped(lane, channels->registry, PLAYER_MSIZE,
            PLAYER_WAIT_MS, PLAYER_TAGS, offer);
        channels->client = styxrpc_named_peer_client(channels->host);
        channels->refs.host = channels->host;
    }
    else
    {
        channels->client = styxclientmountphy(lane, PLAYER_MSIZE, "flash",
            STYXRPC_MODULE_REF_ATTACH, PLAYER_WAIT_MS, PLAYER_TAGS);
        channels->refs.enabled = styxrpc_named_peer_read_ref_scope(channels->client, channels->refs.scope);
    }
    playback->named = channels->client;
    playback->refs = &channels->refs;
    return channels->client != NULL;
}


/* EN: Record both owner roles on the same thread that serves frames and attach requests.
 * RU: Принять обе роли у владельца, обслуживающего запросы attach и кадры. */
static void dis_attach(void *state, const char *name)
{
    FVM_PeerRefs *refs = state;
    if (!strcmp(name, STYXRPC_MODULE_REF_BOTH_ATTACH))
    {
        refs->local_exports_only = 0;
    }
}


/* EN/RU: See player_peer.h. / См. player_peer.h. */
void fvm_styxrpc_player_dis_channels(FVM_StyxRpcPlayback *playback, StyxRpcServer *server, StyxClient *root,
    PlayerPeerChannels *channels)
{
    if (styxrpc_named_peer_read_ref_scope(root, channels->refs.scope))
    {
        channels->refs.slot = playback->slot;
        channels->refs.lane = STYXRPC_MODULE_REF_FORWARD;
        channels->refs.enabled = 1;
        channels->refs.local_exports_only = 1;
        playback->named = root;
        playback->refs = &channels->refs;
        styxrpc_server_observe_attach(server, dis_attach, &channels->refs);
    }
}


/* EN/RU: See player_peer.h. / См. player_peer.h. */
void fvm_styxrpc_player_peer_close(PlayerPeerChannels *channels)
{
    if (channels->host)
    {
        styxrpc_named_peer_destroy(channels->host);
    }
    else
    {
        styxclientshutdown(channels->client);
        styxclientfree(channels->client);
    }
    styxrpc_named_destroy(channels->registry);
}


/* EN/RU: See player_peer.h. / См. player_peer.h. */
int fvm_styxrpc_player_peer_movie(FVM_StyxRpcPlayback *playback, const char *path)
{
    FILE *file = fopen(path, "rb");
    unsigned char *bytes = NULL;
    long length;
    int ok = 0;
    if (!file)
    {
        return 0;
    }
    if (!fseek(file, 0, SEEK_END) && (length = ftell(file)) > 0 &&
        (unsigned long)length <= playback->spec.upload_limit && !fseek(file, 0, SEEK_SET))
    {
        size_t size = (size_t)length;
        bytes = malloc(size);
        if (bytes && fread(bytes, 1, size, file) == size && fgetc(file) == EOF && !ferror(file))
        {
            ok = fvm_styxrpc_playback_start_local(playback, bytes, size);
            bytes = NULL;
        }
    }
    free(bytes);
    fclose(file);
    return ok;
}
