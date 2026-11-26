/* Copyright (c) 2026 bhgv
 * EN: Bootstrap on complementary lanes of one parent-supplied connection.
 * RU: Запуск на встречных каналах одного выданного соединения.
 */
#ifndef FLASHVM_STYXRPC_PLAYER_PEER_H
#define FLASHVM_STYXRPC_PLAYER_PEER_H
#include "playback_internal.h"
#include "client.h"
#include "mux/bidir.h"
#include "styxrpc/named_peer.h"

#define PLAYER_MSIZE 4096U   /* Requested message size in bytes. */
#define PLAYER_WAIT_MS 5000U /* Per-operation bound of the paired client. */
#define PLAYER_TAGS 8U       /* Pending tags on the paired client. */
#define PLAYER_SERVE_FORWARD 0U /* Playback lane when serving a Dis mount. */
#define PLAYER_SERVE_REVERSE 1U /* Complementary lane of the second FlashVM. */

typedef struct PlayerPeerChannels
{
    StyxRpcNamedRegistry *registry; /* Owned only by the authoritative side. */
    StyxRpcNamedPeer *host;          /* Owns the authoritative native endpoints. */
    StyxClient *client;             /* Borrowed from host or owned on the reverse side. */
    FVM_PeerRefs refs;               /* Borrowed by any active movie until peer close. */
} PlayerPeerChannels;


/* EN: Consume the client lane. Serve requests on the owner while the lane
 * mounts. Set named when the peer advertises support on its own attach.
 * Return an owned mount or NULL after the mount worker has joined.
 * RU: Принять клиентский канал. Обслуживать запросы у владельца, пока канал
 * монтируется. Записать named, если второй конец объявил поддержку при attach.
 * Вернуть свой mount или NULL после завершения рабочего потока. */
StyxClient *fvm_styxrpc_player_peer_mount(StyxRpcServer *server, RemotePhy *lane, int *named);


/* EN: Attach the negotiated named service. The forward slot provides the
 * live namespace, and the other side mounts the service's independent lane.
 * Keep playback and mux alive through peer close. channels is initially zero.
 * RU: Подключить согласованную службу имён. Прямой слот даёт живое пространство,
 * вторая сторона монтирует отдельный поток сервиса. Сохранять playback и mux
 * до закрытия peer. channels изначально обнулён. */
int fvm_styxrpc_player_peer_channels(FVM_StyxRpcPlayback *playback, PhyBidir *mux,
    unsigned serve_lane, PlayerPeerChannels *channels);


/* EN: Stop the channel service and close its fids before the playback slot.
 * RU: Остановить службу каналов и закрыть её fid до освобождения слота плеера. */
void fvm_styxrpc_player_peer_close(PlayerPeerChannels *channels);


/* EN: Read the given local startup path and stage it on the playback owner.
 * RU: Прочитать локальный стартовый путь и подготовить фильм у владельца. */
int fvm_styxrpc_player_peer_movie(FVM_StyxRpcPlayback *playback, const char *path);
#endif
