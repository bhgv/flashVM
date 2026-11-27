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

#define PLAYER_PROGRAM_ROOT_SLOTS 2U /* Shared root and hidden .prog. */
#define PLAYER_PROGRAM_TASK_SLOTS 6U /* One process directory, standard children, and .unibus. */
#define PLAYER_PROGRAM_SLOTS (PLAYER_PROGRAM_ROOT_SLOTS + FVM_ASYNC_MAX_TASKS * PLAYER_PROGRAM_TASK_SLOTS)


/* EN: Own reverse program resources for one native connection.
 * RU: Владеть ресурсами встречных программ одного нативного соединения. */
typedef struct PlayerPrograms
{
    StyxRpcPrograms registry;                     /* Dispatcher-owned reverse tree. */
    StyxRpcRegistration nodes[PLAYER_PROGRAM_SLOTS]; /* Fixed storage valid through server destroy. */
    FVM_StyxRpcWorkerScopes scopes;                 /* Owned worker buses until workers and fids retire. */
} PlayerPrograms;

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


/* EN: Publish the connection's worker identities and argument buses before dispatch.
 * Keep programs at its original address until all workers and server fids retire.
 * RU: Выдать встречный PID задач и шину аргументов до запуска диспетчера.
 * Хранить programs по исходному адресу до завершения задач и закрытия fid. */
int fvm_styxrpc_player_programs(FVM_StyxRpcPlayback *playback, PlayerPrograms *programs);


/* EN: Free initialized worker buses after closing all workers and server fids.
 * RU: Освободить созданные шины после завершения всех задач и закрытия fid сервера. */
void fvm_styxrpc_player_programs_close(PlayerPrograms *programs);


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


/* EN: Probe the ordinary Dis mount for scope and observe its forward attach capability.
 * RU: Проверить область в обычном маунте Dis и принять возможность из его прямого attach. */
void fvm_styxrpc_player_dis_channels(FVM_StyxRpcPlayback *playback, StyxRpcServer *server, StyxClient *root,
    PlayerPeerChannels *channels);


/* EN: Stop the channel service and close its fids before the playback slot.
 * RU: Остановить службу каналов и закрыть её fid до освобождения слота плеера. */
void fvm_styxrpc_player_peer_close(PlayerPeerChannels *channels);


/* EN: Read the given local startup path and stage it on the playback owner.
 * RU: Прочитать локальный стартовый путь и подготовить фильм у владельца. */
int fvm_styxrpc_player_peer_movie(FVM_StyxRpcPlayback *playback, const char *path);
#endif
