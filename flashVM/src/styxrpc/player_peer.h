/* Copyright (c) 2026 bhgv
 * EN: Bootstrap on complementary lanes of one parent-supplied connection.
 * RU: Запуск на встречных каналах одного выданного соединения.
 */
#ifndef FLASHVM_STYXRPC_PLAYER_PEER_H
#define FLASHVM_STYXRPC_PLAYER_PEER_H
#include "playback_internal.h"
#include "client.h"

#define PLAYER_MSIZE 4096U   /* Requested message size in bytes. */
#define PLAYER_WAIT_MS 5000U /* Per-operation bound of the paired client. */
#define PLAYER_TAGS 8U       /* Pending tags on the paired client. */
#define PLAYER_SERVE_FORWARD 0U /* Playback lane when serving a Dis mount. */
#define PLAYER_SERVE_REVERSE 1U /* Complementary lane of the second FlashVM. */


/* EN: Consume the client lane. Serve requests on the owner while the lane
 * mounts. Returns an owned mount or NULL after the mount worker has joined.
 * RU: Принять клиентский канал. Обслуживать запросы у владельца, пока канал
 * монтируется. Вернуть свой mount или NULL после завершения рабочего потока. */
StyxClient *fvm_styxrpc_player_peer_mount(StyxRpcServer *server, RemotePhy *lane);


/* EN: Read the given local startup path and stage it on the playback owner.
 * RU: Прочитать локальный стартовый путь и подготовить фильм у владельца. */
int fvm_styxrpc_player_peer_movie(FVM_StyxRpcPlayback *playback, const char *path);
#endif
