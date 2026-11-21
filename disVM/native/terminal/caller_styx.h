/* Copyright (c) 2026 bhgv
 * EN: Native mount binding for a dedicated reverse Styx session.
 * RU: Привязка нативного маунта к встречной сессии Styx.
 */
#ifndef STYX_CALLER_SERVER_H
#define STYX_CALLER_SERVER_H
#include "phy.h"
#include "client.h"
#include "provider.h"

typedef struct StyxCallerServer StyxCallerServer; /* Owned server and binding state. */


/* EN: Consume the reverse transport and return an owned server.
 * RU: Принять встречный транспорт и вернуть сервер. */
StyxCallerServer *styxcallerserver(RemotePhy *reverse);


/* EN: Bind a fresh unpublished forward provider. The provider pins the server state.
 * RU: Привязать свежий неопубликованный провайдер. Провайдер удерживает состояние сервера. */
int styxcallerbind(StyxCallerServer *server, DVM_Provider *forward);


/* EN: Consume the attached forward client and return a bound ordinary mount.
 * Caller retains the reverse server control. The mount retains binding state.
 * RU: Принять прямой клиент и вернуть маунт со встречной связью.
 * Управление остаётся у вызывающего. Маунт удерживает состояние связи. */
DVM_Provider *styxcallermount(StyxCallerServer *server, StyxClient *forward, unsigned access);


/* EN: Join and consume the control after carrier shutdown. Existing captures
 * keep the stopped state alive and can no longer start calls.
 * RU: Дождаться сервера после закрытия транспорта и освободить управление.
 * Захваченные узлы удерживают остановленное состояние, вызовы отклоняются. */
void styxcallerjoin(StyxCallerServer *server);
#endif
