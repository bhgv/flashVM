/* Copyright (c) 2026 bhgv
 * EN: Native mount binding for a dedicated reverse Styx session.
 * RU: Привязка нативного маунта к встречной сессии Styx.
 */
#ifndef STYX_CALLER_SERVER_H
#define STYX_CALLER_SERVER_H
#include "phy.h"
#include "client.h"
#include "provider.h"
#include "tree.h"

typedef struct StyxCallerServer StyxCallerServer; /* Owned server and binding state. */


/* EN: Consume the reverse transport and return an owned server.
 * RU: Принять встречный транспорт и вернуть сервер. */
StyxCallerServer *styxcallerserver(RemotePhy *reverse);


/* EN: Start the same reverse server with an optional retained ordinary view beside its process portal.
 * RU: Запустить тот же встречный сервер с удерживаемым обычным деревом рядом с порталом процессов. */
StyxCallerServer *styxcallerserver_view(RemotePhy *reverse, DVM_Provider *shared);


/* EN: Start a mutable shared view. Borrow callback state until the server joins.
 * RU: Запустить изменяемое общее дерево. Состояние колбэка живёт до join сервера. */
StyxCallerServer *styxcallerserver_view_create(RemotePhy *reverse, DVM_Provider *shared,
    StyxRemoveNode remove, void *state);


/* EN: Start a mutable shared view with one exact root service at a reserved Qid.
 * Borrow the name through join and retain the service's file provider.
 * RU: Запустить изменяемое дерево с точной корневой службой на выделенном Qid.
 * Имя заимствовать до join, файловый провайдер удержать. */
StyxCallerServer *styxcallerserver_view_service(RemotePhy *reverse, DVM_Provider *shared,
    StyxRemoveNode remove, void *state, const char *name, size_t qid, DVM_Provider *service);


/* EN: Pin live server state through local reference resolution. Release without joining.
 * RU: Удержать живое состояние сервера для ссылок. Снять удержание без join. */
int styxcallerretain(StyxCallerServer *server);
void styxcallerrelease(StyxCallerServer *server);


/* EN: Return an owned open for an ordinary Qid already issued at this exact path.
 * Use a retained server. mode accepts 0 for read or 2 for read/write.
 * RU: Вернуть собственное открытие обычного Qid, ранее выданного по точному пути.
 * Сервер удержан вызывающим. mode равен 0 для чтения или 2 для чтения и записи. */
StyxOpen *styxcalleropen_shared(StyxCallerServer *server, const char *path, uint64_t qid, int mode);


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
