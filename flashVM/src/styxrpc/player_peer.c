/* Copyright (c) 2026 bhgv
 * EN: Mount a peer without blocking the opposite Styx server's owner.
 * RU: Монтирование peer без блокировки владельца встречного сервера Styx.
 */
#include "player_peer.h"
#include "../../ports/thread_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define PLAYER_PEER_POLL_MS 20 /* Maximum owner wait between mount checks. */


/* EN: The mount worker owns lane. The owner reads result after joining.
 * RU: Поток монтирования владеет каналом. Владелец читает результат после join. */
typedef struct PeerMount
{
    RemotePhy *lane;      /* Consumed by the blocking Styx mount. */
    pthread_mutex_t lock; /* Protects the completion notification. */
    int done;             /* Mount has completed, including possible failure. */
    StyxClient *result;   /* Owned client, transferred after worker termination. */
} PeerMount;


/* EN: Negotiate the client lane, then publish completion without entering AS.
 * RU: Подключить клиентский канал и опубликовать завершение без входа в AS. */
static void mount_lane(void *state)
{
    PeerMount *mount = state;
    mount->result = styxclientmountphy(mount->lane, PLAYER_MSIZE, "flash", "", PLAYER_WAIT_MS, PLAYER_TAGS);
    pthread_mutex_lock(&mount->lock);
    mount->done = 1;
    pthread_mutex_unlock(&mount->lock);
}


/* EN/RU: See player_peer.h. / См. player_peer.h. */
StyxClient *fvm_styxrpc_player_peer_mount(StyxRpcServer *server, RemotePhy *lane)
{
    PeerMount mount;
    FVM_PortThread *worker;
    int serving = 1;
    if (!lane || pthread_mutex_init(&mount.lock, NULL))
    {
        phyfree(lane);
        return NULL;
    }
    mount.lane = lane;
    mount.done = 0;
    mount.result = NULL;
    worker = fvm_port_thread_start(mount_lane, &mount);
    if (!worker)
    {
        phyfree(lane);
    }
    else
    {
        for (;;)
        {
            int done;
            pthread_mutex_lock(&mount.lock);
            done = mount.done;
            pthread_mutex_unlock(&mount.lock);
            if (done || !serving)
            {
                break;
            }
            serving = styxrpc_server_step_for(server, PLAYER_PEER_POLL_MS);
        }
        fvm_port_thread_join(worker);
    }
    pthread_mutex_destroy(&mount.lock);
    if (!serving)
    {
        styxclientshutdown(mount.result);
        styxclientfree(mount.result);
        return NULL;
    }
    return mount.result;
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
